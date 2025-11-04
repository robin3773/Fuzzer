# Differential Testing: Spike vs DUT

This quick reference shows how to run the Verilator DUT side-by-side with Spike so you can triage mismatches or confirm fixes.

## 1. Prerequisites

- `spike` and an objcopy capable of producing RV32 little-endian ELFs. The project defaults expect:
  - `SPIKE_BIN=/opt/riscv/bin/spike`
  - `OBJCOPY_BIN=riscv32-unknown-elf-objcopy`
  - optional `PK_BIN` if you prefer to launch Spike with the proxy kernel.
- Harness (`afl/afl_picorv32`) and mutator (`afl/isa_mutator/libisa_mutator.so`) built. You can build everything with:

  ```bash
  make -C afl build
  ```

## 2. One-off seed replay

`tools/replay_golden.sh` replays a single binary and performs instruction-by-instruction comparison against Spike. It writes all artefacts into `workdir/replay-<timestamp>/`.

```bash
./tools/replay_golden.sh seeds/firmware.bin
```

The script honours several environment variables:

- `GOLDEN_MODE` (default `live`) – leave at `live` for on-the-fly comparison, or set to `off` to disable Spike while still gathering traces.
- `SPIKE_BIN`, `SPIKE_ISA`, `PK_BIN`, `OBJCOPY_BIN` – override tool paths.
- `MAX_CYCLES` – change the retire limit (defaults to the harness setting, e.g. `50000`).

If the run times out (e.g., tight loops), you can increase the cycle budget:

```bash
MAX_CYCLES=200000 ./tools/replay_golden.sh seeds/asm_nop_loop.bin
```

Results to inspect:

- `replay.log` – combined harness + Spike output.
- `logs/crash/*.log` – detailed failure reports (`golden_divergence_*`, `timeout`, …).
- `traces/` – per-instruction traces when `TRACE_MODE` is `on`.

## 3. Batch regression over a corpus

To run every seed in a directory and stop on the first divergence:

```bash
for seed in seeds/*.bin; do
  echo "=== ${seed} ==="
  if ! ./tools/replay_golden.sh "$seed"; then
    echo "[FAIL] $seed"
    break
  fi
  echo
  sleep 1
done
```

The loop exits as soon as the harness reports a differential failure (non-zero exit code). Logs remain under `workdir/replay-*` per seed for triage.

## 4. Fuzzing with live differential checking

`run.sh` already wires Spike in when `GOLDEN_MODE=live` (default) and `SPIKE_BIN` is reachable. A typical session:

```bash
./run.sh --cores 4 --strategy HYBRID --spike /opt/riscv/bin/spike \
         --objcopy $(which riscv32-unknown-elf-objcopy) --max-cycles 50000
```

The harness converts each fuzz input into a temporary ELF, launches Spike, and compares commit-by-commit. On divergence it records a crash log (`golden_divergence_*`) along with DUT/Spike traces.

## 5. Troubleshooting

- **Spike stops early**: check `workdir/.../spike.log` (or the tail printed in stderr). Ensure `SPIKE_ISA` matches the corpus (e.g., `rv32imc`).
- **Objcopy failures**: override `OBJCOPY_BIN` or install 32-bit/newlib toolchain. The harness falls back to `riscv64-unknown-elf-objcopy` if available.
- **Timeouts**: raise `MAX_CYCLES`, or treat as legitimate (the DUT retired fewer instructions than expected due to loops or hangs).
- **Noise from compressed instructions**: keep `RV32_ENABLE_C=1` to align with Spike when feeding `rv32imc` workloads.

With these commands you can quickly validate DUT changes, capture mismatches, or gate commits with Spike-based regression sweeps.
