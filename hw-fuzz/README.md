# 🧠 Hardware Fuzzing Framework (Verilator + AFL++)

This project provides a modular template for building and fuzzing hardware CPU cores (like PicoRV32 or VexRiscv).

## Directory Overview
```
rtl/        → CPU RTL and testbench
harness/    → C++ fuzzing harness & Makefile
tools/      → build / run / triage scripts
afl/        → AFL++ config and mutators
seeds/      → initial test corpus
corpora/    → fuzzer outputs (queue, crashes)
traces/     → waveforms and logs
docs/       → design notes
```

## Quickstart
```bash
chmod +x tools/build.sh
./tools/build.sh top      # build verilator binary
./tools/run_fuzz.sh       # start fuzzing
```

## Next Steps
- Place your CPU RTL under `rtl/cpu/`
- Add `rtl/top.sv` wrapper exposing clk, rst_n, pc, trap, imem[]
- Write harness/fuzz_picorv32.cpp
