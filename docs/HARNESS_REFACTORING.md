# HarnessMain.cpp Refactoring Summary

## Problem
The original `HarnessMain.cpp` was cluttered with 551 lines containing:
- Helper functions mixed with main logic
- 200+ lines of ELF building code inline
- 150+ lines of golden model differential checking inline
- Poor separation of concerns
- Difficult to maintain and test

## Solution: Module Organization

### New Files Created

#### 1. **SpikeHelpers.hpp/cpp**
Extracted Spike-related utility functions:
- `print_log_tail()` - Print last N lines of spike.log
- `build_spike_elf()` - Build temporary ELF from raw binary input
- `format_arg()` - Safely format command-line arguments

**Benefits:**
- Reusable Spike utilities
- Testable in isolation
- Cleaner main file

#### 2. **GoldenModel.hpp/cpp**
Encapsulates all Spike golden model logic:
- Initialization from environment
- Process lifecycle management
- Trace file management
- Error handling and logging

**Key Methods:**
- `initialize()` - Setup Spike with config from environment
- `next_commit()` - Get next commit with error handling
- `stop()` - Clean shutdown and cleanup
- `write_trace()` - Optional golden trace recording

**Benefits:**
- Single responsibility (golden model management)
- Hides Spike process complexity
- Automatic cleanup in destructor

#### 3. **DifferentialChecker.hpp/cpp**
Dedicated differential testing module:
- Shadow regfile tracking (x0-x31)
- Shadow CSR tracking (mcycle, minstret)
- Granular divergence checks

**Check Methods:**
- `check_pc_divergence()` - PC mismatch detection
- `check_regfile_divergence()` - Register file comparison
- `check_memory_divergence()` - Memory operation comparison
- `check_csr_divergence()` - CSR value comparison

**Benefits:**
- Organized differential logic
- Detailed error messages
- Easy to add new checks
- Testable independently

#### 4. **HarnessMain_NEW.cpp**
Clean, organized main file (300 lines vs 551):

**Structure:**
```cpp
// ============================================================================
// Signal Handling
// ============================================================================
install_signal_handlers()

// ============================================================================
// Input Loading
// ============================================================================
load_input()

// ============================================================================
// Trace Setup
// ============================================================================
setup_trace()

// ============================================================================
// DUT Execution Loop
// ============================================================================
run_execution_loop()
  ├─ handle_signal_crash()
  ├─ check_exit_conditions()
  └─ Differential checking

// ============================================================================
// Main Entry Point
// ============================================================================
main()
  ├─ Setup configuration
  ├─ Initialize DUT
  ├─ Setup golden model
  ├─ Run execution loop
  └─ Handle termination
```

**Benefits:**
- Clear section separation
- Self-documenting structure
- Easy to navigate
- Focused responsibilities

## File Size Comparison

| File | Before | After | Change |
|------|--------|-------|--------|
| HarnessMain.cpp | 551 lines | 300 lines | **-45%** |
| SpikeHelpers.cpp | - | 185 lines | NEW |
| GoldenModel.cpp | - | 140 lines | NEW |
| DifferentialChecker.cpp | - | 190 lines | NEW |

**Total:** 551 lines → 815 lines (distributed across 4 focused modules)

## Code Quality Improvements

### Before (Cluttered):
```cpp
// 200 lines of ELF building inline in main()
char tmpbin[] = "/tmp/dut_in_XXXXXX.bin";
int bfd = ::mkstemps(tmpbin, 4);
// ... 200 more lines ...

// 150 lines of differential checking inline
if (golden_ready) {
  CommitRec g;
  if (spike.next_commit(g)) {
    if (rec.pc_w != g.pc_w) {
      // ... 20 lines of error handling ...
    }
    // ... 130 more lines ...
  }
}
```

### After (Clean):
```cpp
// Golden model setup - 1 line
golden.initialize(input, trace_dir);

// Differential check - 1 line
diff_checker.check_divergence(rec, gold_rec, logger, cyc, input);
```

## Testing Benefits

Each module can now be tested independently:

1. **SpikeHelpers** - Test ELF building with mock inputs
2. **GoldenModel** - Test Spike lifecycle with mock processes
3. **DifferentialChecker** - Test divergence detection with known diffs
4. **HarnessMain** - Integration test with mocked modules

## Maintainability Improvements

### Adding New Features:
- **New crash check?** → Add to `CrashDetection` module
- **New differential check?** → Add to `DifferentialChecker`
- **New Spike feature?** → Extend `GoldenModel` class
- **New helper?** → Add to `SpikeHelpers` or `Utils`

### Debugging:
- Clear module boundaries
- Focused logging per module
- Easy to trace execution flow

## Migration Plan

1. ✅ Create new module files
2. ✅ Implement HarnessMain_NEW.cpp
3. ⚠️  **Next:** Update Makefile to build new files
4. ⚠️  **Next:** Test compilation
5. ⚠️  **Next:** Run smoke tests
6. ⚠️  **Next:** Replace HarnessMain.cpp → backup old, rename new

## Makefile Updates Needed

Add to build system:
```makefile
HARNESS_SRCS += \
  src/SpikeHelpers.cpp \
  src/GoldenModel.cpp \
  src/DifferentialChecker.cpp
```

## Summary

**Before:** Monolithic 551-line file with mixed concerns
**After:** 4 focused modules with clear responsibilities

**Key Wins:**
- 45% reduction in main file complexity
- Better testability
- Clearer code organization
- Easier maintenance
- Reusable components
