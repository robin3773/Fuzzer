# Verilator Coverage Integration

## Overview

This document describes the integration of Verilator's structural coverage metrics with AFL++ fuzzing to guide test generation towards better hardware coverage.

## AFL++ Custom Mutator Optional Symbols

When AFL++ loads a custom mutator library (like `libisa_mutator.so`), it searches for various API functions. The following warnings are **normal and expected**:

```
[*] optional symbol 'afl_custom_post_run' not found.
[*] optional symbol 'afl_custom_queue_new_entry' not found
[*] optional symbol 'afl_custom_describe' not found.
```

### Why These Are Optional

AFL++ defines multiple custom mutator API functions, but not all are required:

**Required Functions:**
- `afl_custom_init` - Initialize the mutator
- `afl_custom_mutator` or `afl_custom_fuzz` - Core mutation function
- `afl_custom_deinit` - Cleanup

**Optional Functions:**
- `afl_custom_post_run` - Called after each test case execution (useful for feedback)
- `afl_custom_queue_new_entry` - Called when AFL++ adds a new test case to the queue
- `afl_custom_describe` - Provides human-readable description of mutations
- `afl_custom_havoc_mutation` - Alternative mutation strategy
- `afl_custom_trim` - Input minimization

Our ISA mutator implements only the required functions plus `afl_custom_havoc_mutation`. The warnings simply indicate that the optional functions are not implemented, which is perfectly fine.

## Coverage Metrics

Verilator provides several types of structural coverage that complement AFL++'s control-flow coverage:

- **Line Coverage**: Tracks which RTL lines have been executed
- **Toggle Coverage**: Tracks signal bit transitions (0→1, 1→0)  
- **FSM Coverage**: Tracks state machine state transitions (if annotated)

By integrating these metrics with AFL++, the fuzzer can:
1. Discover inputs that exercise new hardware paths
2. Find corner cases that toggle rare signal combinations
3. Reach hard-to-trigger hardware states

## Enabling Coverage

### Build with Coverage

To enable Verilator coverage, rebuild with the `ENABLE_COVERAGE=1` flag:

```bash
cd afl
make clean
make ENABLE_COVERAGE=1 -j$(nproc)
```

This adds the following to the build:
- `--coverage-line`: Line coverage instrumentation
- `--coverage-toggle`: Toggle coverage instrumentation  
- `-DVM_COVERAGE=1`: Enables coverage API in harness
- Links `verilated_coverage.cpp`

### Run with Coverage

Coverage is automatically collected during fuzzing when enabled. The coverage data is written to:

```
workdir/coverage.dat
```

### Analyze Coverage

After fuzzing, analyze the coverage report:

```bash
# Generate human-readable coverage report
verilator_coverage --annotate coverage_report workdir/coverage.dat

# View annotated RTL with coverage metrics
ls coverage_report/
# Annotated_picorv32.v  - RTL with coverage annotations
# coverage.txt          - Summary statistics
```

### Example: Annotated RTL

The annotated RTL shows line hit counts and toggle coverage:

```verilog
000001 // picorv32.v - Line executed 1 time
000000 // picorv32.v - Line never executed
T01/01 wire mem_valid;  // Toggle: 0->1 (yes), 1->0 (yes)
T00/01 wire rare_sig;   // Toggle: 0->1 (no),  1->0 (yes)
```

## Performance Impact

Coverage instrumentation adds overhead:

- **Line Coverage**: ~10-15% slowdown
- **Toggle Coverage**: ~20-30% slowdown
- **Combined**: ~30-40% slowdown

### Recommendation

Enable coverage periodically rather than continuously:

1. **Initial fuzzing**: Run without coverage for maximum speed
2. **Coverage analysis**: Enable coverage for 1-2 hours to identify gaps
3. **Targeted fuzzing**: Use coverage report to guide manual test generation
4. **Repeat**: Periodically re-enable to measure progress

## Coverage-Guided Fuzzing Strategy

### Phase 1: Fast Discovery (No Coverage)
```bash
make clean && make -j$(nproc)
./run.sh --cores $(nproc) --timeout 100000
# Run for 24 hours
```

### Phase 2: Coverage Analysis
```bash
make clean && make ENABLE_COVERAGE=1 -j$(nproc)
./run.sh --cores 4 --timeout 200000
# Run for 2 hours with reduced cores due to overhead
```

### Phase 3: Review Gaps
```bash
verilator_coverage --annotate coverage_report workdir/coverage.dat
# Identify uncovered lines and signals
# Create targeted seeds for uncovered areas
```

### Phase 4: Targeted Testing
```bash
# Add seeds that target uncovered areas
# Resume fast fuzzing
make clean && make -j$(nproc)
./run.sh --cores $(nproc)
```

## Coverage Metrics

### Line Coverage
Shows percentage of RTL lines executed:
- **Goal**: >80% for mature testing
- **Typical**: 60-70% with automated fuzzing
- **Gaps**: Reset logic, error handlers, rare conditions

### Toggle Coverage  
Shows percentage of signal bits that toggled both directions:
- **Goal**: >70% for data paths
- **Typical**: 40-60% with automated fuzzing
- **Gaps**: Status flags, overflow bits, error signals

### Combined Coverage
The product of line and toggle coverage indicates overall exercised functionality:
- **High line, low toggle**: Code runs but doesn't explore data variations
- **Low line, high toggle**: Missing code paths but good data diversity
- **Both high**: Strong coverage of both control and data

## Integration with AFL++

The `VerilatorCoverage` class provides:

1. **Automatic tracking**: Coverage accumulates during execution
2. **Delta reporting**: Only new coverage reported to AFL++
3. **Feedback integration**: Coverage metrics influence AFL++ seed selection

### How It Works

```cpp
// In HarnessMain.cpp
fuzz::VerilatorCoverage coverage;
coverage.initialize("workdir/coverage.dat");

// Run simulation
run_execution_loop(..., coverage, ...);

// Write coverage and compute deltas
coverage.write_and_reset();
```

During execution:
- Verilator automatically updates coverage counters
- After each test case, coverage is written to disk
- Deltas (new lines/toggles) are computed
- AFL++ uses deltas to prioritize interesting inputs

## Troubleshooting

### Coverage not working
- Check `VM_COVERAGE` is defined: `make ENABLE_COVERAGE=1`
- Verify coverage file exists: `ls -lh workdir/coverage.dat`
- Check logs for: `[COVERAGE] Verilator structural coverage enabled`

### No coverage output
- Ensure execution completes (not hanging/crashing early)
- Check file permissions on `workdir/`
- Verify Verilator version supports coverage: `verilator --version`

### Coverage file too large
- Coverage accumulates over time, growing large
- Periodically archive: `mv workdir/coverage.dat archive/coverage_$(date +%s).dat`
- Start fresh: `rm workdir/coverage.dat`

## Advanced: FSM Coverage

For state machine coverage, annotate FSM state variables in RTL:

```verilog
/* verilator coverage_block_off */
typedef enum {IDLE, FETCH, DECODE, EXECUTE} state_t;
/* verilator coverage_block_on */

(* fsm_state *) state_t current_state;
```

Then enable FSM coverage:
```bash
make ENABLE_COVERAGE=1 VERILATOR_FLAGS="--coverage-line --coverage-toggle --coverage-user"
```

## References

- [Verilator Coverage Documentation](https://veripool.org/guide/latest/exe_verilator_coverage.html)
- [AFL++ Custom Feedback](https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/custom_mutators.md)
- [Hardware Fuzzing Best Practices](../README.md)
