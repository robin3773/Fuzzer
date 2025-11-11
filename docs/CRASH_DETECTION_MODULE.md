# Crash Detection Module

## Overview

The crash detection module (`CrashDetection.hpp` / `CrashDetection.cpp`) extracts all RISC-V specification violation checks into a separate, reusable component. This improves code organization and maintainability.

## Location

- **Header**: `afl_harness/include/CrashDetection.hpp`
- **Implementation**: `afl_harness/src/CrashDetection.cpp`
- **Namespace**: `crash_detection::`

## Provided Functions

All functions follow the same pattern:
1. Check if RVFI interface is valid
2. Extract relevant signals
3. Validate RISC-V specification rules
4. Log crash if violation detected
5. Return true if crash occurred, false otherwise

### check_x0_write()

```cpp
bool check_x0_write(const CpuIface* cpu, const CrashLogger& logger, 
                   unsigned cyc, const std::vector<unsigned char>& input)
```

**Purpose**: Validates that register x0 (zero register) always reads as zero.

**RISC-V Rule**: x0 is hardwired to zero and writes to it are discarded.

**Crash Type**: `"x0_write"`

### check_pc_misaligned()

```cpp
bool check_pc_misaligned(const CpuIface* cpu, const CrashLogger& logger,
                        unsigned cyc, const std::vector<unsigned char>& input)
```

**Purpose**: Ensures PC is properly aligned.

**RISC-V Rule**: PC must be at minimum 2-byte aligned (4-byte for RV32I without C extension).

**Crash Type**: `"pc_misaligned"`

### check_mem_align_load()

```cpp
bool check_mem_align_load(const CpuIface* cpu, const CrashLogger& logger,
                          unsigned cyc, const std::vector<unsigned char>& input)
```

**Purpose**: Validates load memory access alignment and byte mask patterns.

**RISC-V Rules**:
- Byte masks must be contiguous (1, 3, or 15)
- Half-word (2 bytes) loads must be 2-byte aligned
- Word (4 bytes) loads must be 4-byte aligned

**Crash Types**: `"mem_mask_irregular_load"`, `"mem_unaligned_load"`

### check_mem_align_store()

```cpp
bool check_mem_align_store(const CpuIface* cpu, const CrashLogger& logger,
                           unsigned cyc, const std::vector<unsigned char>& input)
```

**Purpose**: Validates store memory access alignment and byte mask patterns.

**RISC-V Rules**:
- Byte masks must be contiguous (1, 3, or 15)
- Half-word (2 bytes) stores must be 2-byte aligned
- Word (4 bytes) stores must be 4-byte aligned

**Crash Types**: `"mem_mask_irregular_store"`, `"mem_unaligned_store"`

## Usage in HarnessMain.cpp

The main execution loop calls these functions on each cycle where RVFI reports a valid instruction retirement:

```cpp
// Perform retire-time checks via dedicated functions
if (crash_detection::check_x0_write(cpu, logger, cyc, input)) _exit(1);
if (crash_detection::check_pc_misaligned(cpu, logger, cyc, input)) _exit(1);
if (crash_detection::check_mem_align_store(cpu, logger, cyc, input)) _exit(1);
if (crash_detection::check_mem_align_load(cpu, logger, cyc, input)) _exit(1);
```

If any check detects a violation:
1. Crash details are written via `CrashLogger::writeCrash()`
2. The function returns `true`
3. The harness exits with code 1 (AFL++ interprets this as a crash)

## Build Integration

The module is compiled as part of the AFL++ harness build:

**In `afl/Makefile`:**
```makefile
HARNESS_SRCS := \
    $(HARNESS_SRC_DIR)/HarnessMain.cpp \
    $(HARNESS_SRC_DIR)/CpuPicorv32.cpp \
    $(HARNESS_SRC_DIR)/SpikeProcess.cpp \
    $(HARNESS_SRC_DIR)/DutExit.cpp \
    $(HARNESS_SRC_DIR)/SpikeExit.cpp \
    $(HARNESS_SRC_DIR)/CrashDetection.cpp \
    $(TOP_DIR)/include/hwfuzz/Debug.cpp
```

## Dependencies

- **CpuIface.hpp**: Interface for accessing RVFI signals
- **CrashLogger.hpp**: For recording crash details with objdump disassembly
- **<vector>**: For input data storage
- **<cstdint>**: For fixed-width integer types

## Benefits

1. **Modularity**: Crash detection logic separated from main execution loop
2. **Reusability**: Functions can be used in other harness implementations
3. **Maintainability**: Single location for all crash detection logic
4. **Documentation**: Comprehensive Doxygen comments explain each check
5. **Testing**: Easier to unit test individual crash detection functions

## Related Documentation

- See `CRASH_DETECTION.md` for overall crash handling architecture
- See `differential_testing.md` for Spike golden model comparison
- See `exit_stub_design.md` for normal termination detection
