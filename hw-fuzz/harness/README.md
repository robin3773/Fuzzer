# Harness Directory

Contains Verilator C++ fuzzing harness and Makefile.

- `fuzz_picorv32.cpp`: main harness entry (loads @@ input, runs CPU)
- `harness_common.h`: shared utils and macros
- `Makefile`: builds Verilator binary (`Vtop`)
