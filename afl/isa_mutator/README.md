
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
- `fuzz/isa/IsaLoader.hpp` — YAML loader for ISA schema files.

### src/
- `ISAMutator.cpp` — implementation of the schema-driven mutation pipeline.
- `IsaLoader.cpp` — YAML parsing and `ISAConfig` construction.
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
MUTATOR_CONFIG=./afl/isa_mutator/config/mutator.default.yaml \
afl-fuzz -i afl/seeds -o afl/corpora -- ./afl/afl_picorv32 @@
```

## Configuration

All mutator settings are configured via YAML file specified by `MUTATOR_CONFIG` environment variable.

### YAML Configuration File
- **Default**: `afl/isa_mutator/config/mutator.default.yaml`
- **Resolution**: `MUTATOR_CONFIG` env → `mutator.default.yaml` → built-in defaults
- **Sample profiles**: 
  - `mutator.default.yaml` - Balanced settings (IR strategy, no compressed)
  - `mutator.aggressive.yaml` - High mutation rates
  - `mutator.riscv32e.yaml` - RV32E ISA variant

### YAML Fields
```yaml
strategy: IR              # RAW | IR | HYBRID | AUTO
verbose: true             # Enable startup config logging
enable_c: false           # Enable compressed instructions

probabilities:
  decode: 60              # 0-100: schema-guided mutation chance (HYBRID/AUTO)
  imm_random: 25          # 0-100: random immediate vs delta mutation

weights:
  r_base_alu: 70          # 0-100: ALU instruction weight
  r_m: 30                 # 0-100: M-extension weight

schemas:
  isa: rv32im             # ISA name (maps to schemas/riscv/rv32*.yaml)
  root: ./schemas         # Schema directory root
  map: isa_map.yaml       # ISA mapping file
```

### Environment Variables
- `MUTATOR_CONFIG` - Path to YAML config file (required)
- `SCHEMA_DIR` - Override schema directory root
- `MUTATOR_EFFECTIVE_CONFIG` - Path to dump effective config after loading
- `DEBUG_MUTATOR` - Master debug switch: enables all logging, function tracing, and debug file output (0/1)

**Note**: When `DEBUG_MUTATOR=1`, the mutator automatically:
- Enables verbose logging to stdout (via harness_log)
- Enables function entry/exit tracing
- Creates debug log file at `afl/isa_mutator/logs/mutator_debug.log`
- Logs illegal instruction mutations for analysis
