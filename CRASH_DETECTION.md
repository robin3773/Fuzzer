# AFL++ Crash Detection for Golden Model Mismatches

## Problem
Previously, when the DUT diverged from the Spike golden model, the harness would:
- Log the mismatch to a crash file
- Exit with `_exit(123)` (normal exit with return code)
- **AFL++ did NOT detect this as a crash** ❌

This meant AFL++ couldn't effectively learn which inputs caused hardware bugs!

## Solution
Changed all golden model mismatch exits from `_exit(123)` to `std::abort()`.

### Why `std::abort()`?
- Sends **SIGABRT** signal (abnormal termination)
- AFL++ detects signals (SIGABRT, SIGSEGV, SIGILL, etc.) as crashes
- Crash gets saved to `crashes/` directory for analysis
- AFL++ **learns from crashes** and prioritizes similar mutations 🎯

## Changes Made

Updated all mismatch detection points in `HarnessMain.cpp`:

### 1. PC Mismatch
```cpp
if (rec.pc_w != g.pc_w) {
  // ... log crash details ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}
```

### 2. Register File Mismatch
```cpp
if (first_diff != -1) {
  // ... log register differences ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}
```

### 3. Memory Operation Mismatch
```cpp
// Memory kind (load vs store)
if ((dut_store != g.mem_is_store) || (dut_load != g.mem_is_load)) {
  // ... log mismatch ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}

// Memory store address
if (dut_store && g.mem_is_store && (rec.mem_addr != g.mem_addr)) {
  // ... log mismatch ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}

// Memory load address
if (dut_load && g.mem_is_load && (rec.mem_addr != g.mem_addr)) {
  // ... log mismatch ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}
```

### 4. CSR Mismatch
```cpp
// minstret counter
if (dut_minstret != gold_minstret) {
  // ... log mismatch ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}

// mcycle counter
if (dut_mcycle != gold_mcycle) {
  // ... log mismatch ...
  std::fprintf(hwfuzz::harness_log(), "[CRASH] %s", oss.str().c_str());
  std::abort();  // ← Changed from _exit(123)
}
```

## How AFL++ Will Use This

### Before (with `_exit(123)`):
```
AFL++ sees: Normal exit, return code 123
AFL++ thinks: "This input completed successfully"
Result: No crash saved, no learning ❌
```

### After (with `std::abort()`):
```
AFL++ sees: Process terminated with SIGABRT
AFL++ thinks: "CRASH DETECTED! 🎯"
Result: 
  1. Input saved to crashes/id:NNNNNN,sig:06,... 
  2. AFL++ marks this input's path as crash-inducing
  3. Fuzzer prioritizes similar mutations
  4. Coverage map updated with crash discovery
```

## Mismatch Types Detected

| Mismatch Type | Crash Name | What It Catches |
|--------------|------------|-----------------|
| PC mismatch | `golden_divergence_pc` | Control flow errors, wrong branch targets |
| Register mismatch | `golden_divergence_regfile` | ALU bugs, data corruption, wrong operands |
| Memory kind | `golden_divergence_mem_kind` | Load/store confusion |
| Memory store addr | `golden_divergence_mem_store_addr` | Store address calculation bugs |
| Memory load addr | `golden_divergence_mem_load_addr` | Load address calculation bugs |
| CSR minstret | `golden_divergence_csr_minstret` | Retired instruction counter bugs |
| CSR mcycle | `golden_divergence_csr_mcycle` | Cycle counter bugs |

## Crash File Locations

When a mismatch occurs:
1. **Crash log**: `${CRASH_DIR}/crash_TIMESTAMP_TYPE.log` (detailed info)
2. **AFL++ crash**: `${CORPORA_DIR}/default/crashes/id:NNNNNN,sig:06,...` (raw input)
3. **Harness log**: `${LOGS_DIR}/harness.log` (contains `[CRASH]` marker)

## Benefits

✅ **AFL++ learns from bugs**: Crashes guide mutation strategy  
✅ **Crash corpus grows**: All bug-triggering inputs saved  
✅ **Faster bug discovery**: Fuzzer prioritizes crash-inducing paths  
✅ **Better coverage**: Crashes reveal unexplored code paths  
✅ **Easy debugging**: Each crash has detailed log with full context  

## Monitoring Crashes

```bash
# View crash count in real-time
./view_fuzzer.sh

# List all crashes
ls -lh workdir/$(ls -t workdir | head -1)/corpora/default/crashes/

# View crash details
cat workdir/$(ls -t workdir | head -1)/logs/crash/crash_*_golden_divergence_*.log

# Replay a crash
afl/afl_picorv32 workdir/.../corpora/default/crashes/id:000000,sig:06,...
```

## Next Steps

The fuzzer will now:
1. ✅ Detect golden model mismatches as crashes (SIGABRT)
2. ✅ Save crash-inducing inputs for analysis
3. ✅ Learn from crashes to find more bugs faster
4. ✅ Build a corpus of DUT-divergent test cases

Run the fuzzer and watch crashes accumulate! 🐛🔍
