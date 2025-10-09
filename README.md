# HW-Fuzz — Verilator + AFL++ Hardware Fuzzing Framework

A practical template and quick-start guide for fuzzing CPU cores (e.g. PicoRV32, VexRiscv) using Verilator and AFL++.
This repo organizes RTL, harnesses, seeds and a single central `workdir/` where runtime logs and artifacts are collected.


---

## Directory layout (short table)

| Path           | Purpose                                                              |
| -------------- | -------------------------------------------------------------------- |
| `rtl/`         | CPU RTL (put cores under `rtl/cpu/`) and sim wrappers (`rtl/top.sv`) |
| `harness/`     | Verilator C++ harnesses, loaders, Makefile                           |
| `tools/`       | Build/run/fuzz wrappers and logging wrappers                         |
| `seeds/`       | Initial corpus for AFL                                               |
| `afl/`         | AFL tuning files and mutator source(s)                               |
| `corpora/`     | AFL outputs (queue, crashes, hangs) — runtime only                   |
| `traces/`      | Waveforms/triage outputs (VCD, logs)                               |
| `workdir/`     | Central working directory with `logs/`, `traces/`, `corpora/`        |
| `docs/`        | Design docs and recipes                                              |
| `experiments/` | Experiment configs/results                                           |
| `verilator/`   | (ignored) Verilator obj_dir and binaries                             |
| `ci/`          | CI scripts / Dockerfile                                              |

---

## Quickstart — get running (assumes required tools installed)

1. Clone repo and enter it:

```bash
git clone <your-repo-url>
cd <repo>
```

2. (Optional but recommended) Create & activate a Python virtualenv for tooling:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip setuptools wheel
pip install pyelftools tabulate
```

3. Place your CPU RTL in `rtl/cpu/` (e.g., `picorv32.v`) and adapt `rtl/top.sv` if needed.

4. Build Verilator simulation:

```bash
# simple build (tool wrapper expects harness + RTL names)
./tools/build.sh top

# or use wrapper that logs to workdir
./tools/build_and_log.sh top
```

5. Run a single input (to test harness):

```bash
./tools/run_once.sh ./obj_dir/Vtop seeds/seed_empty.bin

# or capture logs and traces
./tools/run_once_log.sh ./obj_dir/Vtop seeds/seed_empty.bin
```

6. Start AFL++ fuzzing:

```bash
./tools/run_fuzz_log.sh ./obj_dir/Vtop seeds workdir/corpora
# AFL stdout/stderr will be saved to workdir/logs/afl/
```
