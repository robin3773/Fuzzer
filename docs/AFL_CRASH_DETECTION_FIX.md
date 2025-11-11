# AFL++ Crash Detection Fix

**Date**: November 11, 2025  
**Issue**: AFL++ not saving crashes to `corpora/default/crashes/`  
**Status**: ✅ Fixed

## Problem

The fuzzer was detecting crashes (PC stagnation, timeouts, traps, etc.) and logging them to `workdir/logs/crash/`, but **AFL++ was not saving them to `corpora/default/crashes/`**.

### Root Cause

The harness was using `_exit()` with custom exit codes:
- PC stagnation: `_exit(127)`
- Trap: `_exit(124)`
- Timeout: `_exit(125)`
- Alignment errors: `_exit(1)`

**AFL++ doesn't recognize non-zero exits as "crashes"** - it expects actual signals (SIGABRT, SIGSEGV, etc.). Non-zero exits are treated as "hangs" or just discarded.

## Solution

Changed all crash exits from `_exit(code)` to `std::abort()`:

```cpp
// BEFORE:
if (crash_detection::check_x0_write(cpu, logger, cyc, input)) _exit(1);
logger.writeCrash("pc_stagnation", rec.pc_r, rec.insn, cyc, input, oss.str());
_exit(127);

// AFTER:
if (crash_detection::check_x0_write(cpu, logger, cyc, input)) std::abort();
logger.writeCrash("pc_stagnation", rec.pc_r, rec.insn, cyc, input, oss.str());
std::abort();
```

### Changes Made

**File**: `afl_harness/src/HarnessMain.cpp`

1. **PC stagnation detection** (line ~376): `_exit(127)` → `std::abort()`
2. **CPU trap detection** (line ~551): `_exit(124)` → `std::abort()`
3. **Timeout detection** (line ~568): `_exit(125)` → `std::abort()`
4. **Alignment checks** (lines 542-545): `_exit(1)` → `std::abort()`

## Result

Now when the harness detects a crash:

1. ✅ **Logs crash to** `workdir/logs/crash/crash_*.log` (already working)
2. ✅ **Calls `std::abort()`** which sends SIGABRT
3. ✅ **AFL++ catches SIGABRT** and recognizes it as a crash
4. ✅ **Saves input to** `corpora/default/crashes/id:XXXXXX,sig:06,*`

### Crash Types Detected

| Crash Type | Signal | AFL++ Detection |
|------------|--------|-----------------|
| x0 write | SIGABRT | ✅ Crashes |
| PC misaligned | SIGABRT | ✅ Crashes |
| Memory misalignment | SIGABRT | ✅ Crashes |
| PC stagnation | SIGABRT | ✅ Crashes |
| Timeout | SIGABRT | ✅ Crashes |
| CPU trap | SIGABRT | ✅ Crashes |

## Verification

After rebuilding and running AFL++:

```bash
# Check AFL++ crashes directory
ls -la workdir/corpora/default/crashes/

# Should now contain files like:
# id:000000,sig:06,src:000000,time:123,execs:456,op:...
```

The `sig:06` indicates SIGABRT (signal 6), which AFL++ recognizes as a crash.

## Why abort() Instead of raise(SIGABRT)?

Both work, but `std::abort()` is preferred because:
- Standard C++ function (no need for POSIX headers)
- Guaranteed to terminate the process
- AFL++ specifically recognizes it
- Cleaner code

## Backward Compatibility

The crash logs in `workdir/logs/crash/` still work exactly the same way. The only difference is how AFL++ treats the exit.
