
# RV32 Hybrid Mutator (AFL++) — Modular C++17

This is a **modular, reusable hybrid mutator** for AFL++ that targets **RV32** today and includes a clean **RV64 placeholder** for future expansion. The mutator supports **dynamic strategy** selection via environment variables and includes an **AUTO** mode placeholder (behaves like HYBRID for now).

## What each file does

### Top-level
- `Makefile` — builds `librv32_mutator.so` as a shared library for AFL++.

### include/
- `MutatorConfig.hpp` — runtime configuration (strategy, probabilities, weights) loaded from environment variables.
- `Random.hpp` — fast xorshift32 RNG wrapper used across modules.
- `Instruction.hpp` — common enums & helpers, minimal IR (`IR32`) struct and little-endian IO utilities.
- `RV32Decoder.hpp` — decoder interface for RV32; class with static methods to detect format and decode to `IR32`.
- `RV32Encoder.hpp` — encoder interface for RV32; re-encodes an `IR32` back into a 32-bit word.
- `CompressedMutator.hpp` — safe, conservative mutations for **compressed (C)** 16-bit instructions.
- `RV32Mutator.hpp` — core mutator implementing the **hybrid strategy** (raw + IR) and the buffer mutation driver.
- `AFLInterface.hpp` — C ABI entrypoints that AFL++ expects (`afl_custom_fuzz`, init/deinit, etc.).
- `RV64Placeholder.hpp` — empty placeholder struct and notes to guide adding RV64 later.

### src/
- `Random.cpp` — RNG state definition.
- `Instruction.cpp` — currently header-only helpers (kept for structure/expansion).
- `RV32Decoder.cpp` — implements format detection + decoding to IR for RV32.
- `RV32Encoder.cpp` — implements re-encoding from IR to raw 32-bit words.
- `CompressedMutator.cpp` — implements conservative mutations for compressed instructions.
- `RV32Mutator.cpp` — **the core**: dynamic strategy selection, raw/IR mutation logic, stream mutation driver.
- `AFLInterface.cpp` — binds the C++ mutator to AFL++’s custom mutator API, seeds RNG, loads config.

## Build
```bash
# from this directory (afl/rv32_mutator)
make
# produces: librv32_mutator.so
```

## Use with AFL++ (example)
```bash
AFL_CUSTOM_MUTATOR_LIBRARY=./afl/rv32_mutator/librv32_mutator.so \
RV32_MUT_STRATEGY=hybrid RV32_DECODE_PROB=25 \
afl-fuzz -i afl/seeds -o afl/corpora -- ./afl/afl_picorv32 @@
```

## Runtime knobs (environment variables)
- `RV32_MUT_STRATEGY` = `raw | ir | hybrid | auto` (default: `hybrid`)
- `RV32_DECODE_PROB`  = 0..100 (default: 20) probability of using IR in HYBRID/AUTO
- `RV32_C_ENABLE`     = 0/1 (default: 1) enable compressed mutations
- `RV32E_MODE`        = 0/1 (default: 0) restrict regs to x0..x15
- `RV32_IMM_RANDOM_PROB` = 0..100 (default: 30) random vs delta imm mutation
- `RV32_IMM_DELTA_MAX`   = +/- range for delta imm mutation (default: 16)
- `RV32_R_WEIGHT_BASE`   = % weight for base ALU for R-type (default: 70)
- `RV32_R_WEIGHT_M`      = % weight for M-extension for R-type (default: 30)
- `RV32_I_SHIFT_WEIGHT`  = % bias toward shift-immediate opcodes for OP-IMM (default: 30)
- `RV32_VERBOSE`         = 0/1 prints configuration on init

## Notes
- **AUTO** strategy is a **placeholder**: behaves like HYBRID using `RV32_DECODE_PROB`.
- The mutator is designed to be **ISA-agnostic-friendly**: RV64 can be added by introducing `RV64Decoder/Encoder` and switching by XLEN.
