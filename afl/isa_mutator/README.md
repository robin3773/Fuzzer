
# ISA Mutator (AFL++) — Schema-driven C++17 Library

This directory contains the schema-aware custom mutator that AFL++ loads via `afl_custom_mutator`. The code centers on `ISAMutator`, which consumes YAML ISA descriptions and gracefully falls back to simple heuristic rules when schemas are unavailable.

## Layout

### Top-level
- `Makefile` — builds `libisa_mutator.so` for AFL++.
- `build/` — object files and intermediate artifacts.
- `config/` — default YAML configuration consumed at runtime.

### include/
- `fuzz/mutator/ISAMutator.hpp` — schema-driven mutator entry point.
- `fuzz/mutator/MutatorConfig.hpp` — runtime knobs sourced from config file + environment.
- `fuzz/mutator/Random.hpp` — xorshift-based RNG shared by mutator components.
- `fuzz/mutator/LegalCheck.hpp` — structural checks against ISA constraints.
- `fuzz/mutator/MutatorInterface.hpp` — AFL-facing wrapper helpers.
- `fuzz/mutator/MutatorDebug.hpp` — opt-in logging utilities.
- `fuzz/isa/Loader.hpp` — YAML loader for ISA schema files.

### src/
- `ISAMutator.cpp` — implementation of the schema-driven mutation pipeline.
- `isa_loader.cpp` — YAML parsing and `ISAConfig` construction.
- `AFLInterface.cpp` — hooks exported to AFL++ (`afl_custom_*` symbols).
- `Random.cpp`, `LegalCheck.cpp` — shared utilities backing the mutator.
- `CompressedMutator.cpp` — conservative mutations for RVC lanes.
- `encode_helpers.hpp` — parsing helpers for schema patterns.

### test/
- `mutator_selftest.cpp` — standalone harness that loads the shared object and prints disassembly diffs.
- `Makefile` — builds and runs the self-test against `libisa_mutator.so`.

## Build
```bash
# from this directory (afl/isa_mutator)
make
# produces: libisa_mutator.so
```

## Use with AFL++ (example)
```bash
AFL_CUSTOM_MUTATOR_LIBRARY=./afl/isa_mutator/libisa_mutator.so \
RV32_STRATEGY=HYBRID RV32_DECODE_PROB=25 \
afl-fuzz -i afl/seeds -o afl/corpora -- ./afl/afl_picorv32 @@
```

## Runtime knobs (environment variables)
- `RV32_STRATEGY` = `RAW | IR | HYBRID | AUTO`
- `RV32_DECODE_PROB` = 0..100 chance of schema-guided mutation when in HYBRID/AUTO
- `RV32_ENABLE_C` = 0/1 toggles compressed instruction mutations
- `RV32_IMM_RANDOM` = 0..100 weighting for random immediates vs delta mutations
- `RV32_R_BASE` / `RV32_R_M` = weights for ALU vs M-extension rules
- `RV32_VERBOSE` = 0/1 prints configuration on init
- `MUTATOR_ISA` / `MUTATOR_SCHEMAS` / `MUTATOR_SCHEMA` = control schema selection

The remaining RV32-specific environment variables are accepted for backward compatibility while the schema pipeline matures.

## Configuration file
- Default config lives at `afl/isa_mutator/config/mutator.default.yaml` and mirrors the legacy in-code defaults (legacy `mutator.yaml` is still accepted).
- Resolution order: CLI `--config <path>` → `MUTATOR_CONFIG` env → per-ISA config (`afl/isa_mutator/config/<isa>.yaml`) → `mutator.default.yaml` → built-in defaults.
- The self-test harness forwards `--config` into the shared object via `mutator_set_config_path` before `afl_custom_init` runs.
- Set `MUTATOR_EFFECTIVE_CONFIG=/path/out.yaml` to capture the post-override configuration emitted at startup; the mutator will ensure the parent directory exists.
- Environment variables listed above still win over file-supplied values, enabling per-run tweaks without editing YAML.
- Sample profiles: `mutator.aggressive.yaml`, `mutator.riscv32e.yaml` (extend by dropping more files under `config/`).
