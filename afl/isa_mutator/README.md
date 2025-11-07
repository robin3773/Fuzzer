
# ISA Mutator (AFL++) — Schema-driven C++17 Library

This directory contains the schema-aware custom mutator that AFL++ loads via `afl_custom_mutator`. The code centers on `ISAMutator`, which consumes YAML ISA descriptions and gracefully falls back to simple heuristic rules when schemas are unavailable.

## File Hierarchy

```
afl/isa_mutator/
├── config/                          # Configuration Files
│   ├── mutator.default.yaml         # Default balanced settings (IR strategy)
│   ├── mutator.aggressive.yaml      # High mutation rate profile
│   ├── mutator.riscv32e.yaml        # RV32E ISA variant profile
│   └── mutator.yaml                 # Legacy config (backward compat)
│
├── include/fuzz/                    # Header Files
│   ├── isa/                         # ISA Schema Loading
│   │   ├── IsaLoader.hpp            # ISA schema loader interface
│   │   ├── Loader.hpp               # Generic loader base
│   │   └── YamlUtils.hpp            # YAML parsing utilities
│   │
│   └── mutator/                     # Mutation Engine
│       ├── AFLInterface.hpp         # AFL++ custom mutator API
│       ├── CompressedMutator.hpp    # RVC (compressed) instruction mutations
│       ├── DebugUtils.hpp           # Function tracing and debug helpers
│       ├── EncodeHelpers.hpp        # Instruction encoding utilities
│       ├── ExitStub.hpp             # Exit stub generation (LUI/ADDI/SW/EBREAK)
│       ├── ISAMutator.hpp           # Main mutation engine
│       ├── LegalCheck.hpp           # Instruction legality validation
│       ├── MutatorConfig.hpp        # Configuration loading and management
│       ├── MutatorDebug.hpp         # Debug logging system
│       ├── MutatorInterface.hpp     # Core mutator interface
│       └── Random.hpp               # Random number generation
│
├── src/                             # Implementation Files
│   ├── AFLInterface.cpp             # AFL++ hooks (afl_custom_init, afl_custom_fuzz, etc.)
│   ├── CompressedMutator.cpp        # RVC mutation implementations
│   ├── DebugUtils.cpp               # Debug tracing implementation
│   ├── IsaLoader.cpp                # YAML ISA schema loading
│   ├── ISAMutator.cpp               # Core mutation logic (REPLACE/INSERT/DELETE/DUPLICATE)
│   ├── LegalCheck.cpp               # Instruction validation rules
│   ├── MutatorConfig.cpp            # Config YAML parsing and env handling
│   ├── Random.cpp                   # RNG seeding and utilities
│   └── YamlUtils.cpp                # YAML helper functions
│
├── test/                            # Test Suite
│   ├── mutator_selftest.cpp         # C++ unit tests
│   ├── test_exit_stub_encoding.py   # Python exit stub verification
│   └── Makefile                     # Test build system
│
├── Makefile                         # Build system (produces libisa_mutator.so)
└── README.md                        # This file

Build Artifacts (generated):
├── build/                           # Object files (*.o)
└── libisa_mutator.so                # Shared library for AFL++
```

## Component Architecture

### 1. Entry Point (AFL++ Integration)
```
AFLInterface.cpp / AFLInterface.hpp
├── afl_custom_init()        - Initialize mutator, seed RNG
├── afl_custom_fuzz()        - Main mutation entry point
├── afl_custom_deinit()      - Cleanup
└── mutator_set_config_path() - Set config before init
     │
     └─→ MutatorConfig.cpp / MutatorConfig.hpp
         ├── loadFromFile()      - Parse YAML config
         └── Config struct       - strategy, probabilities, weights
              │
              └─→ config/mutator.default.yaml
                  ├── strategy: IR / HYBRID / RAW / AUTO
                  ├── probabilities: decode, imm_random
                  ├── weights: r_base_alu, r_m
                  └── schemas: isa, root, map
```

### 2. Mutation Engine (Core Logic)
```
ISAMutator.cpp / ISAMutator.hpp
├── mutate()                - Main mutation entry
├── mutate_with_strategy()  - Strategy dispatch
└── Strategies:
    ├── REPLACE  - Overwrite instruction with random valid instruction
    ├── INSERT   - Add new instruction at random offset
    ├── DELETE   - Remove instruction from random offset
    └── DUPLICATE - Copy instruction to random offset
     │
     ├─→ Random.cpp/hpp
     │   ├── gen_u32()   - Generate random 32-bit value
     │   ├── gen_range() - Generate random in range
     │   └── gen_index() - Generate random index
     │
     ├─→ ExitStub.hpp
     │   ├── append_exit_stub()  - Add 5-instruction exit sequence
     │   ├── has_exit_stub()     - Check if stub present
     │   ├── is_tail_locked()    - Protect last 5 instructions
     │   └── Encoders: encode_lui(), encode_addi(), encode_sw()
     │
     └─→ LegalCheck.cpp / LegalCheck.hpp
         ├── is_legal_instruction()      - Validate instruction encoding
         ├── check_register_constraints() - Verify register usage
         └── validate_immediate_ranges()  - Check immediate bounds
```

### 3. ISA Schema System (Instruction Knowledge)
```
IsaLoader.cpp / IsaLoader.hpp
├── load()           - Parse YAML ISA schemas
├── InstructionDef   - Instruction metadata
└── FieldDef         - Instruction field definitions
     │
     └─→ YamlUtils.cpp / YamlUtils.hpp
         ├── parse_node()        - YAML parsing helpers
         └── validate_schema()   - Schema validation
              │
              └─→ ../../schemas/riscv/
                  ├── rv32i.yaml         - Base integer ISA
                  ├── rv32m.yaml         - Multiply/divide extension
                  ├── rv32c.yaml         - Compressed instructions
                  └── riscv32_exit.yaml  - Exit stub schema
```

### 4. Compressed Instructions (RVC)
```
CompressedMutator.cpp/hpp
├── mutate_compressed()  - Apply RVC-aware mutations
├── expand_to_32bit()    - Decompress C.* to full instruction
└── compress_to_16bit()  - Compress eligible instructions
```

### 5. Debug & Utilities
```
MutatorDebug.hpp
├── init_from_env()  - Read DEBUG_MUTATOR env var
├── log_illegal()    - Track illegal mutations
└── State: enabled, log_to_file, fp

DebugUtils.cpp / DebugUtils.hpp
├── FunctionTracer   - RAII function entry/exit logging
└── is_debug_enabled() - Check debug state

EncodeHelpers.hpp
├── encode_r_type()  - R-type instruction encoder
├── encode_i_type()  - I-type instruction encoder
├── encode_s_type()  - S-type instruction encoder
└── encode_u_type()  - U-type instruction encoder
```

## Data Flow During Fuzzing

```
AFL++ Fuzzer
    │
    ├─→ afl_custom_fuzz(data, size, max_size)
    │        │
    │        └─→ ISAMutator::mutate(data, size)
    │                 │
    │                 ├─→ Select strategy (REPLACE/INSERT/DELETE/DUPLICATE)
    │                 │
    │                 ├─→ Random::gen_range() - Pick mutation point
    │                 │
    │                 ├─→ Random::gen_u32() - Generate new instruction
    │                 │
    │                 ├─→ LegalCheck::is_legal() - Validate encoding
    │                 │
    │                 ├─→ Apply mutation to buffer
    │                 │
    │                 └─→ ExitStub::append_exit_stub() - Add 5-instr exit
    │                          │
    │                          └─→ Return mutated buffer
    │
    └─→ Harness executes mutated program
```

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
- `DEBUG_MUTATOR` - Master debug switch: enables all logging, function tracing, and debug file output (0/1)

**Note**: When `DEBUG_MUTATOR=1`, the mutator automatically:
- Enables verbose logging to stdout (via harness_log)
- Enables function entry/exit tracing
- Creates debug log file at `afl/isa_mutator/logs/mutator_debug.log`
- Logs illegal instruction mutations for analysis
