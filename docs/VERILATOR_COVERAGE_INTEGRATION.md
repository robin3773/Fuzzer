# Verilator Coverage Integration with AFL++

## Current Status: ⚠️ **NOT INTEGRATED**

The Verilator coverage is **enabled and compiling**, but it's **NOT being fed to AFL++**!

## Problem

### What's Happening Now:

1. ✅ Verilator coverage compiles with `VM_COVERAGE=1`
2. ✅ Coverage code exists in `VerilatorCoverage.cpp`
3. ❌ **Coverage data is NOT written to file**
4. ❌ **Coverage is NOT fed to AFL++ bitmap**
5. ❌ **AFL++ cannot see hardware coverage**

### Why AFL++ Doesn't See Coverage:

**Current implementation:**
```cpp
void VerilatorCoverage::report_to_afl() {
  // This does NOTHING!
  // Verilator automatically updates coverage counters during simulation
  // We don't need to do anything here - coverage accumulates in background
}
```

**Problem:** The coverage accumulates in Verilator's internal data structures, but is **never written to a file** and **never fed to AFL++'s bitmap** (`__afl_area_ptr`).

## Where is `coverage.dat`?

**Expected location:** `workdir/traces/coverage.dat`

**Actual status:** ❌ **File is never created!**

```bash
$ ls -lh workdir/traces/
total 8.0K
-rw-r--r-- 1 robin robin 402 Nov 13 00:46 dut.trace
-rw-r--r-- 1 robin robin 672 Nov 13 00:46 golden.trace
# ❌ No coverage.dat!
```

## How Coverage SHOULD Work

### AFL++ Coverage Model

AFL++ uses a **shared memory bitmap** (`__afl_area_ptr`) to track coverage:

```
__afl_area_ptr[0..65535]  // 64KB bitmap
Each byte = hit count for a specific edge/feature

AFL++ updates this during execution:
  __afl_area_ptr[hash(cur_pc, prev_pc)]++;
```

### Integration Strategy

We need to **feed Verilator's coverage metrics into AFL++'s bitmap**:

```cpp
extern "C" uint8_t *__afl_area_ptr;  // AFL++'s shared memory

void report_verilator_coverage_to_afl() {
  // Get Verilator coverage metrics
  uint32_t lines_covered = get_lines_hit();
  uint32_t toggles = get_toggles_hit();
  
  // Hash to AFL++ bitmap indices
  uint32_t line_idx = hash(lines_covered) % 65536;
  uint32_t toggle_idx = hash(toggles) % 65536;
  
  // Update AFL++ bitmap
  __afl_area_ptr[line_idx]++;
  __afl_area_ptr[toggle_idx]++;
}
```

## Fix #1: Add AFL++ Bitmap Integration

### Update `Feedback.cpp`:

```cpp
#include "Feedback.hpp"
#include <hwfuzz/Debug.hpp>
#include <cstdint>

// AFL++'s coverage bitmap (automatically available)
extern "C" uint8_t *__afl_area_ptr;

Feedback::Feedback() 
  : afl_area_(nullptr), afl_map_size_(0), prev_pc_(0) {
}

Feedback::~Feedback() {
  // AFL++ manages the shared memory lifecycle
}

void Feedback::initialize() {
  // Get AFL++'s shared memory pointer
  afl_area_ = __afl_area_ptr;
  afl_map_size_ = 65536;  // Standard AFL++ map size
  
  if (afl_area_) {
    hwfuzz::debug::logInfo("[FEEDBACK] AFL++ bitmap available at %p\n", afl_area_);
  } else {
    hwfuzz::debug::logWarn("[FEEDBACK] AFL++ bitmap not available (not running under AFL++)\n");
  }
  
  prev_pc_ = 0;
}

void Feedback::report_instruction(uint32_t pc, uint32_t insn) {
  if (!afl_area_) return;
  
  // Hash PC edge transition
  uint32_t edge = (prev_pc_ ^ pc) * 0x9E3779B1;  // Knuth hash
  uint32_t idx = edge % afl_map_size_;
  
  // Update AFL++ bitmap
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
  
  prev_pc_ = pc;
}

void Feedback::report_memory_access(uint32_t addr, bool is_write) {
  if (!afl_area_) return;
  
  // Hash memory access pattern
  uint32_t access_hash = (addr ^ (is_write ? 0xAAAAAAAA : 0x55555555)) * 0x9E3779B1;
  uint32_t idx = access_hash % afl_map_size_;
  
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
}

void Feedback::report_register_write(uint32_t reg_num, uint32_t value) {
  if (!afl_area_) return;
  
  // Hash register write pattern
  uint32_t write_hash = ((reg_num << 24) ^ value) * 0x9E3779B1;
  uint32_t idx = write_hash % afl_map_size_;
  
  if (afl_area_[idx] < 255) {
    afl_area_[idx]++;
  }
}

// NEW: Feed Verilator coverage to AFL++
void Feedback::report_verilator_coverage(uint32_t lines, uint32_t toggles, 
                                          uint32_t fsm_states) {
  if (!afl_area_) return;
  
  // Map each coverage metric to different parts of AFL++ bitmap
  uint32_t line_idx = (lines * 0x9E3779B1) % afl_map_size_;
  uint32_t toggle_idx = (toggles * 0xDEADBEEF) % afl_map_size_;
  uint32_t fsm_idx = (fsm_states * 0xCAFEBABE) % afl_map_size_;
  
  // Update bitmap
  if (afl_area_[line_idx] < 255) afl_area_[line_idx]++;
  if (afl_area_[toggle_idx] < 255) afl_area_[toggle_idx]++;
  if (afl_area_[fsm_idx] < 255) afl_area_[fsm_idx]++;
}
```

### Update `VerilatorCoverage.cpp`:

```cpp
void VerilatorCoverage::report_to_afl(Feedback& feedback) {
  if (!enabled_) return;

#ifdef VM_COVERAGE
  // Get current coverage counts
  uint32_t total_lines = prev_line_count_;
  uint32_t total_toggles = prev_toggle_count_;
  uint32_t total_fsm = prev_fsm_count_;
  
  // Feed to AFL++ via Feedback
  feedback.report_verilator_coverage(total_lines, total_toggles, total_fsm);
#endif
}
```

## Fix #2: Actually Write `coverage.dat`

### Problem: Coverage is never written to disk

The `write_and_reset()` function is never called in the main loop!

### Solution: Call it after each test case

In `HarnessMain.cpp`, the execute loop should be:

```cpp
auto execute_test_case = [&](const std::vector<unsigned char>& input) {
  // ... reset, load input, run simulation ...
  
  // Write coverage after test case
  coverage.write_and_reset();  // ← ADD THIS!
  
  return detected_crash;
};
```

## Fix #3: Monitor Coverage Growth

### Create a coverage monitoring script:

**File: `tools/monitor_coverage.sh`**
```bash
#!/bin/bash

COVERAGE_FILE="workdir/traces/coverage.dat"

echo "Monitoring Verilator coverage growth..."
echo "Coverage file: $COVERAGE_FILE"
echo

while true; do
  if [ -f "$COVERAGE_FILE" ]; then
    # Parse coverage.dat
    LINES=$(grep "^C 0" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    TOGGLES=$(grep "^C 1" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    FSM=$(grep "^C 2" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    
    echo "$(date '+%H:%M:%S') | Lines: $LINES | Toggles: $TOGGLES | FSM: $FSM"
  else
    echo "$(date '+%H:%M:%S') | Waiting for coverage.dat..."
  fi
  
  sleep 5
done
```

### Or use Verilator's built-in tools:

```bash
# Generate HTML coverage report
verilator_coverage --annotate coverage_html workdir/traces/coverage.dat

# View summary
verilator_coverage --rank workdir/traces/coverage.dat
```

## Current Coverage Sources

Your fuzzer actually HAS coverage, just not from Verilator:

### 1. ✅ **Compiler Instrumentation** (Working)

AFL++'s compiler automatically instruments the C++ harness code:
- Control flow in `HarnessMain.cpp`
- Function calls
- Branches in differential checker

This is why AFL++ finds new paths even without explicit coverage calls.

### 2. ✅ **DUT Hardware Feedback** (Working)

Your `Feedback::report_instruction()` is called for each RVFI commit:
```cpp
// In HarnessMain.cpp execute loop
feedback.report_instruction(rec.pc_r, rec.insn);
```

This provides **instruction-level hardware coverage**:
- PC transitions
- Memory access patterns
- Register write patterns

### 3. ❌ **Verilator Coverage** (NOT working)

Verilator's structural coverage is:
- ✅ Compiled into the simulation
- ❌ Not written to file
- ❌ Not fed to AFL++
- ❌ Not monitored

## How to Enable Full Coverage

### Step 1: Check Current Coverage is Working

```bash
# Run a short fuzzing session
timeout 60 ./tools/run_fuzz.sh

# Check AFL++ is finding new paths
cat workdir/corpora/default/fuzzer_stats | grep paths_total
# Should show increasing numbers
```

### Step 2: Verify Feedback is Active

```bash
# Check if feedback functions are called
nm afl/afl_picorv32 | grep report_instruction
# Should show: Feedback::report_instruction

# Run with debug
AFL_DEBUG=1 ./tools/run_once.sh seeds/seed_arithmetic.bin
# Should see [FEEDBACK] messages
```

### Step 3: Add Verilator Coverage (Optional)

Verilator coverage adds **RTL-level metrics**:
- Line coverage: Which Verilog lines executed
- Toggle coverage: Which signals changed
- FSM coverage: Which state machine states visited

**To enable:**
1. Implement the fixes above
2. Rebuild: `make clean && make fuzz-build`
3. Run fuzzer and check `workdir/traces/coverage.dat` is created

## Monitoring Coverage During Fuzzing

### Method 1: AFL++ Stats (Already working!)

```bash
# Watch AFL++ UI
./tools/run_with_ui.sh

# Key metrics:
# - map density: % of bitmap filled (increases = more coverage)
# - new edges on: New code paths found
# - corpus count: Number of interesting inputs
```

### Method 2: Parse Coverage File

```bash
# If coverage.dat exists, analyze it
verilator_coverage --rank workdir/traces/coverage.dat | head -20

# Or custom script
watch -n 5 'grep "^C 0" workdir/traces/coverage.dat | wc -l'
```

### Method 3: Log Monitoring

```bash
# Watch for coverage deltas in logs
tail -f workdir/logs/runtime.log | grep COVERAGE
# Should see: [COVERAGE] Delta: +X lines, +Y toggles
```

## Summary

### Current State:

| Coverage Type | Status | Source | Fed to AFL++? |
|--------------|--------|--------|---------------|
| **Compiler Instrumentation** | ✅ Working | AFL++ automatic | ✅ Yes |
| **DUT Instruction Tracking** | ✅ Working | `Feedback::report_instruction()` | ⚠️ Maybe* |
| **Memory Access Patterns** | ✅ Working | `Feedback::report_memory_access()` | ⚠️ Maybe* |
| **Verilator Line Coverage** | ❌ Not working | `VerilatorCoverage` | ❌ No |
| **Verilator Toggle Coverage** | ❌ Not working | `VerilatorCoverage` | ❌ No |

*The Feedback functions are called, but they don't actually update `__afl_area_ptr` yet!

### To Fix:

1. **Add `__afl_area_ptr` updates** in `Feedback.cpp` (see Fix #1)
2. **Call `coverage.write_and_reset()`** in main loop (see Fix #2)
3. **Monitor coverage** with scripts or Verilator tools (see Fix #3)

### Bottom Line:

**You have coverage from:**
- ✅ AFL++ compiler instrumentation (C++ code paths)
- ⚠️ Partial DUT hardware feedback (function calls but not bitmap updates)

**You DON'T have coverage from:**
- ❌ Verilator structural coverage (RTL line/toggle/FSM)

**This is why AFL++ still finds bugs** - the compiler instrumentation alone provides enough coverage signal. But adding proper hardware feedback and Verilator coverage will make it **much more effective**!
