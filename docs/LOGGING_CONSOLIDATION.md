# Logging System Consolidation

**Date**: November 11, 2025  
**Status**: ✅ Complete

## Overview

Consolidated the dual logging system into a **single unified log file** (`runtime.log`) that captures all output from both the ISA mutator and AFL harness in chronological order.

## Problem

Previously, there were **two separate logging systems**:

1. **`harness.log`** (380MB) - Created by `hwfuzz::harness_log()` from `Log.hpp`
   - Used by both ISA mutator and AFL harness
   - Controlled by `HARNESS_STDIO_LOG` environment variable
   - Required stdio redirection in HarnessMain.cpp
   - Always enabled

2. **`runtime.log`** (48KB) - Created by `hwfuzz::debug::` from `Debug.cpp`
   - Used for debug-level tracing
   - Only enabled when `DEBUG=1`
   - Minimal usage in the codebase

This caused:
- Confusion about which log to check
- Logs scattered across two files
- Difficult to see the chronological order of operations
- Extra complexity with stdio redirection

## Solution

Migrated everything to use the **`hwfuzz::debug::`** logging system with these changes:

### 1. Made logging always-on
- **Before**: `hwfuzz::debug::log*()` only wrote logs when `DEBUG=1`
- **After**: Logs are always written to `runtime.log`
- **Function tracing**: Still controlled by `DEBUG=1` (to reduce noise)

### 2. Updated all logging calls
Replaced all instances of:
```cpp
std::fprintf(hwfuzz::harness_log(), "[INFO] ...")
std::fprintf(hwfuzz::harness_log(), "[WARN] ...")
std::fprintf(hwfuzz::harness_log(), "[ERROR] ...")
```

With:
```cpp
hwfuzz::debug::logInfo(...)
hwfuzz::debug::logWarn(...)
hwfuzz::debug::logError(...)
```

### 3. Files modified

#### Core logging system:
- **`include/hwfuzz/Debug.cpp`**: Made logging always-on (removed `g_debug_enabled` check)
- **`include/hwfuzz/Debug.hpp`**: Updated documentation
- **`include/hwfuzz/Log.hpp`**: Deprecated with migration instructions

#### ISA Mutator:
- **`afl/isa_mutator/src/ISAMutator.cpp`**: Updated 3 logging calls, removed `Log.hpp` include

#### AFL Harness:
- **`afl_harness/include/Utils.hpp`**: Updated `ensure_dir()` logging
- **`afl_harness/include/HarnessConfig.hpp`**: Updated `load_from_env()` logging (8 calls)
- **`afl_harness/src/HarnessMain.cpp`**: 
  - Removed `redirect_stdio_to_log()` function (no longer needed)
  - Removed `Log.hpp` include
  - Removed stdio redirection logic

#### Build system:
- **`run.sh`**: 
  - Removed `HARNESS_STDIO_LOG` environment variable
  - Removed from `AFL_KEEP_ENV` list
  - Updated status display to show `runtime.log` instead of `harness.log`
  
- **`fuzzer.env`**: Updated comments to reflect single log file

## Result

### Single Log File
All logs now go to: **`workdir/logs/runtime.log`**

This contains:
- ISA mutator initialization and operations
- AFL harness execution logs
- Error messages from both components
- Info and warning messages
- Function traces (when `DEBUG=1`)

### Log Format
```
=== Runtime session started (pid=37153) ===
[Fn Start  ] ISAMutator.cpp::ISAMutator::initFromEnv
[INFO] [MUTATOR] Loaded config: /home/robin/HAVEN/Fuzz/afl/isa_mutator/config/mutator.default.yaml
[INFO] [MUTATOR] Strategy=INSTRUCTION_LEVEL Isa_name=rv32im
[INFO] PROJECT_ROOT: /home/robin/HAVEN/Fuzz
[INFO] Schema directory: /home/robin/HAVEN/Fuzz/schemas
[INFO] Using ISA Map: /home/robin/HAVEN/Fuzz/schemas/isa_map.yaml
[INFO] Loading rv32_base.yaml: 0 instructions
[INFO] Loading rv32i.yaml: 39 instructions
[INFO] Loading rv32m.yaml: 8 instructions
[Fn End    ] ISAMutator.cpp::ISAMutator::initFromEnv
[INFO] [MUTATOR] Custom mutator initialized. pid=37153 time=1762845202

=== Runtime session started (pid=37186) ===
[ERROR] [HARNESS] Failed to start Spike.
  Command: /opt/riscv/bin/spike -l --isa=rv32im /tmp/dut_in_IoKAI0.bin.elf
  ELF: /tmp/dut_in_IoKAI0.bin.elf
[ERROR] [HARNESS]   See Spike log: /home/robin/HAVEN/Fuzz/workdir/spike.log
```

### DEBUG Flag Behavior

- **`DEBUG=0`** (default): 
  - INFO/WARN/ERROR messages logged
  - Function traces **disabled**
  - Keeps log file size manageable

- **`DEBUG=1`**:
  - INFO/WARN/ERROR messages logged
  - Function traces **enabled** (`[Fn Start]` / `[Fn End]`)
  - Useful for debugging specific issues

### Viewing Logs

```bash
# View latest runtime log
tail -f workdir/logs/runtime.log

# View with timestamps
less workdir/logs/runtime.log

# Search for errors
grep '\[ERROR\]' workdir/logs/runtime.log

# See mutator-specific logs
grep '\[MUTATOR\]' workdir/logs/runtime.log

# See harness-specific logs
grep '\[HARNESS\]' workdir/logs/runtime.log
```

## Benefits

1. ✅ **Single source of truth**: All logs in one chronological file
2. ✅ **Simplified debugging**: No need to check multiple log files
3. ✅ **Better correlation**: See mutator and harness operations in sequence
4. ✅ **Cleaner code**: Removed stdio redirection complexity
5. ✅ **Always-on logging**: No more missed logs due to DEBUG=0
6. ✅ **AFL++ friendly**: stdout/stderr remain clean for AFL++ UI

## Backward Compatibility

- **`Log.hpp`**: Still exists but deprecated (shows migration instructions)
- **`hwfuzz::harness_log()`**: Still functional but not used
- **Old code**: Will compile but should be migrated

## Migration Guide for External Code

If you have external code using the old system:

```cpp
// OLD (deprecated):
#include <hwfuzz/Log.hpp>
std::fprintf(hwfuzz::harness_log(), "[INFO] Message: %s\n", str);

// NEW (recommended):
#include <hwfuzz/Debug.hpp>
hwfuzz::debug::logInfo("Message: %s\n", str);
```

No need to include `[INFO]` prefix - it's added automatically.

## Testing

All components compile successfully:
- ✅ ISA mutator: `afl/isa_mutator/libisa_mutator.so`
- ✅ AFL harness: `afl/afl_picorv32`
- ✅ No compilation errors
- ✅ No lint errors

## Files to Remove (Future)

These files are no longer needed but kept for backward compatibility:
- `include/hwfuzz/Log.hpp` - Can be removed once all external code is migrated
- Old documentation references to `harness.log`
