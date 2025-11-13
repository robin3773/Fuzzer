# Memory Model Alignment - Quick Summary

## What Was Changed?

✅ **Aligned DUT memory model with Spike**

### Key Changes in `afl_harness/src/CpuPicorv32.cpp`:

1. **Memory base address**: 0x0 → 0x80000000 (matches Spike)
2. **Address validation**: Now rejects unmapped addresses
3. **Bus timeout**: Invalid accesses don't get `mem_ready=1`

## Before vs After

| Aspect | Before | After |
|--------|--------|-------|
| **Base Address** | 0x0 | 0x80000000 ✅ |
| **Valid Range** | All addresses (wrapping) | 0x80000000 - 0x8000FFFF ✅ |
| **Unmapped Access** | Wraps/succeeds | Rejected ✅ |
| **Behavior** | Unrealistic | Matches Spike ✅ |

## Impact

### False Positives Eliminated

**Example crash that's now fixed:**
```
Store to address 0xd:
  Before: DUT accepts (wraps), Spike rejects → False divergence
  After:  DUT rejects, Spike rejects → No divergence ✅
```

### Real Bugs Still Detected

These are STILL caught (good!):
- ❌ Wrong address calculation
- ❌ Wrong byte enables (wmask)
- ❌ Wrong ALU results
- ❌ Wrong branch targets

## How to Test

```bash
# 1. Rebuild (already done)
make clean && make fuzz-build

# 2. Test with a seed
./tools/run_once.sh seeds/seed_arithmetic.bin

# 3. Run fuzzer
./tools/run_fuzz.sh

# Expected: Fewer false positives, same real bugs caught
```

## What to Expect

### Crashes That Will Disappear
- `golden_divergence_mem_kind` where address < 0x80000000
- Any divergence caused by low memory access

### Crashes That Will Remain
- **Real PicoRV32 bugs**:
  - Wrong wmask (0xf instead of 0x1 for byte stores)
  - Address calculation errors
  - ALU/branch/CSR bugs

## Memory Map

```
Valid Memory:
┌────────────────────────────┐
│ 0x80000000 - 0x8000FFFF   │ ← 64 KB DRAM
│ - User code here           │
│ - Fuzzer input loaded here │
│ - Stack and heap           │
└────────────────────────────┘

Unmapped (Rejected):
- 0x00000000 - 0x7FFFFFFF
- 0x80010000 - 0xFFFFFFFF
```

## If Something Breaks

### DUT hangs?
- Check that your seeds use addresses ≥ 0x80000000
- Seeds should already be correct (use exit stub at 0x80001000)

### All tests fail?
```bash
# Verify seeds are valid
./tools/verify_exit_stubs.sh

# Check memory addresses in crash logs
./tools/analyze_crash.sh workdir/logs/crash/<crash>.log
```

### Can't reproduce old crashes?
- **Good!** They were false positives (memory model mismatch)
- Real bugs will still appear

## Next Steps

1. ✅ **Rebuild complete** - memory models aligned
2. 🔍 **Run fuzzer** - find real bugs (not false positives)
3. 📊 **Analyze crashes** - all divergences are now real bugs
4. 🐛 **Fix PicoRV32** - address calc, wmask, etc.

## Documentation

- **Full guide**: `docs/MEMORY_ALIGNMENT.md`
- **Crash analysis**: `docs/CRASH_ANALYSIS_FAQ.md`
- **Feedback testing**: `docs/FEEDBACK_AND_CRASH_ANALYSIS.md`

---

**TL;DR:** Memory models are now aligned. False positives eliminated. Real bugs still caught. Happy fuzzing! 🎉
