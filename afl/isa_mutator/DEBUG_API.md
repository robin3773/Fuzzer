# Centralized Debug API

## Overview

The `fuzz::debug` namespace provides a unified logging interface for both the ISA mutator and AFL harness. All logging respects the `DEBUG` environment variable and outputs to a log file only.

## Environment Variable

- **`DEBUG=1`**: Enables all debug logging to file
- **`DEBUG=0`** or unset: No logging output

## Log Output

When `DEBUG=1`:
- All messages logged to **`afl/isa_mutator/logs/mutator_debug.log`**
- **No output to stderr** (keeps AFL++ output clean)

When `DEBUG=0`:
- No log file is created
- No output anywhere

## API Functions

### Header
```cpp
#include <fuzz/Debug.hpp>
```

### Functions

#### `logInfo(const char* fmt, ...)`
Log informational messages. Only logs to file if `DEBUG=1`.

```cpp
fuzz::debug::logInfo("Loaded config: %s\n", config_path);
fuzz::debug::logInfo("Mutation strategy: %d\n", strategy);
```

#### `logWarn(const char* fmt, ...)`
Log warning messages. Only logs to file if `DEBUG=1`.

```cpp
fuzz::debug::logWarn("Using fallback ISA schema\n");
fuzz::debug::logWarn("Probability %d clamped to %d\n", value, clamped);
```

#### `logError(const char* fmt, ...)`
Log error messages. Only logs to file if `DEBUG=1`.

```cpp
fuzz::debug::logError("Failed to load config: %s\n", path);
fuzz::debug::logError("Invalid instruction encoding: 0x%08x\n", instr);
```

#### `logDebug(const char* fmt, ...)`
Log detailed debug traces. Only logs to file if `DEBUG=1`.

```cpp
fuzz::debug::logDebug("Mutating instruction at offset %zu\n", offset);
fuzz::debug::logDebug("Register allocation: rs1=%d rs2=%d rd=%d\n", rs1, rs2, rd);
```

#### `isDebugEnabled()`
Check if debug mode is active.

```cpp
if (fuzz::debug::isDebugEnabled()) {
    // Expensive debug operation only when needed
    dumpInstructionTrace();
}
```

#### `getDebugLog()`
Get the debug log file handle (for custom formatting).

```cpp
FILE* log = fuzz::debug::getDebugLog();
if (log) {
    std::fprintf(log, "Custom debug output\n");
}
```

## Usage Examples

### Example 1: Replace printf/fprintf
**Before:**
```cpp
std::fprintf(stderr, "[INFO] Loaded config: %s\n", path);
```

**After:**
```cpp
fuzz::debug::logInfo("Loaded config: %s\n", path);
```

### Example 2: Conditional debug output
**Before:**
```cpp
if (std::getenv("DEBUG") && strcmp(std::getenv("DEBUG"), "1") == 0) {
    std::fprintf(stderr, "[DEBUG] Instruction: 0x%08x\n", instr);
}
```

**After:**
```cpp
fuzz::debug::logDebug("Instruction: 0x%08x\n", instr);
```

### Example 3: Error handling
**Before:**
```cpp
std::fprintf(stderr, "[ERROR] Failed to parse YAML: %s\n", err.what());
std::exit(1);
```

**After:**
```cpp
fuzz::debug::logError("Failed to parse YAML: %s\n", err.what());
std::exit(1);
```

## Migration Guide

### For ISA Mutator Code

Replace:
- `std::fprintf(stderr, "[INFO] ...")` → `fuzz::debug::logInfo(...)`
- `std::fprintf(stderr, "[WARN] ...")` → `fuzz::debug::logWarn(...)`
- `std::fprintf(stderr, "[ERROR] ...")` → `fuzz::debug::logError(...)`
- `std::fprintf(stderr, "[DEBUG] ...")` → `fuzz::debug::logDebug(...)`

### For AFL Harness Code

Replace:
- `std::fprintf(hwfuzz::harness_log(), "[INFO] ...")` → `fuzz::debug::logInfo(...)`
- `std::fprintf(hwfuzz::harness_log(), "[WARN] ...")` → `fuzz::debug::logWarn(...)`
- `std::fprintf(hwfuzz::harness_log(), "[ERROR] ...")` → `fuzz::debug::logError(...)`

## Benefits

1. **Unified Interface**: Same API for mutator and harness
2. **Automatic Filtering**: No manual `if (debug_enabled)` checks needed
3. **File-Only Output**: All logs to file, keeps AFL++ UI clean
4. **Thread-Safe**: Mutex-protected output
5. **Performance**: Debug flag cached, zero overhead when disabled
6. **Clean Code**: No `[INFO]`, `[ERROR]` prefixes to type manually

## Thread Safety

All logging functions are thread-safe through internal mutex locking. You can safely call them from multiple threads.

## AFL++ Integration

Since all output goes to the log file (not stderr), this keeps the AFL++ fuzzer UI clean and doesn't interfere with AFL++'s stdout/stderr handling. To view logs while fuzzing:

```bash
# In another terminal
tail -f afl/isa_mutator/logs/mutator_debug.log
```
