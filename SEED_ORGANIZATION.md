# Seed Organization and Exit Stub Status

## Directory Structure

```
HAVEN/Fuzz/
├── seeds/               # General fuzzing seeds (5 files)
│   ├── asm_branch_calc.bin
│   ├── asm_mem_rw.bin
│   ├── asm_nop_loop.bin
│   ├── firmware.bin
│   └── quick_exit.bin
│
└── test_seeds/          # Targeted bug detection tests (8 files)
    ├── test_add_carry_bug.bin
    ├── test_beq_bug.bin
    ├── test_or_bug.bin
    ├── test_sll_bug.bin
    ├── test_sll_bug_simple.bin
    ├── test_store_mask_bug.bin
    ├── test_xor_bug.bin
    └── test_xor_bug_simple.bin
```

## Exit Stub Verification

All seeds have been verified to contain proper exit stubs using the tohost mechanism.

### Verification Command
```bash
./verify_exit_stubs.sh
```

### Exit Stub Pattern
All seeds use this standard exit sequence:
```assembly
# Exit via tohost
lui  x5, 0x80001    # Load base address
li   x6, 1          # Exit code 1
sw   x6, 0(x5)      # Store to 0x80001000 (tohost)
1:  j 1b            # Infinite loop
```

### Verification Results

#### Regular Seeds (seeds/)
| File | Exit Stub | Notes |
|------|-----------|-------|
| asm_branch_calc.bin | ✅ Present | Branch calculation tests |
| asm_mem_rw.bin | ✅ Present | Memory read/write operations |
| asm_nop_loop.bin | ✅ Present | NOP loop for timing |
| firmware.bin | ✅ Present | Firmware initialization |
| quick_exit.bin | ✅ Present | **Fixed** - Was corrupted, now rebuilt |

#### Test Seeds (test_seeds/)
| File | Exit Stub | Bug Tested | Detection Status |
|------|-----------|------------|------------------|
| test_add_carry_bug.bin | ✅ Present | Bug #3: ADD carry | ✅ Detected |
| test_beq_bug.bin | ✅ Present | Bug #4: BEQ inversion | ✅ Detected |
| test_or_bug.bin | ✅ Present | Bug #7: OR bit 31 | ✅ Detected |
| test_sll_bug.bin | ✅ Present | Bug #6: SLL shift | ⚠️ Not testable |
| test_sll_bug_simple.bin | ✅ Present | Bug #6: SLL shift | ⚠️ Not testable |
| test_store_mask_bug.bin | ✅ Present | Bug #1 & #2: SB/SH | ⏸️ Blocked |
| test_xor_bug.bin | ✅ Present | Bug #5: XOR bit 0 | ✅ Detected |
| test_xor_bug_simple.bin | ✅ Present | Bug #5: XOR bit 0 | ✅ Detected |

**Status**: ✅ 13/13 seeds verified (100%)

## Seed Usage

### Regular Seeds (seeds/)
**Purpose**: General fuzzing corpus
- Used for exploratory fuzzing campaigns
- Diverse instruction coverage
- Realistic code patterns
- Suitable for AFL++ mutation

**Usage**:
```bash
# Persistent mode fuzzing
PERSISTENT_MODE=on AFL_CUSTOM_MUTATOR_LIBRARY=./afl/isa_mutator/libisa_mutator.so \
  afl-fuzz -i seeds/ -o workdir/corpora -- ./afl/afl_picorv32 @@

# One-shot execution
./afl/afl_picorv32 seeds/asm_branch_calc.bin
```

### Test Seeds (test_seeds/)
**Purpose**: Targeted bug validation
- Specific bug triggering
- Differential checking validation
- Security analysis
- Regression testing

**Usage**:
```bash
# Test with differential checking
GOLDEN_MODE=live ./afl/afl_picorv32 test_seeds/test_add_carry_bug.bin

# Expect crash with golden_divergence error
# Check workdir/logs/crash/ for details
```

## Exit Stub Importance

The exit stub (tohost mechanism) is critical for:

1. **Proper Termination**: Signals test completion to harness
2. **Coverage Tracking**: Ensures AFL++ can measure execution paths
3. **Timeout Prevention**: Prevents infinite loops from hanging fuzzer
4. **Result Validation**: Differentiates successful runs from hangs
5. **Differential Testing**: Synchronizes DUT and golden model completion

### Without Exit Stub
- Test appears to hang (infinite execution)
- AFL++ timeout kills process (slow fuzzing)
- No clear termination signal
- Coverage data incomplete

### With Exit Stub
- Clean termination in microseconds
- AFL++ sees complete execution
- Full coverage information collected
- Differential checking completes properly

## Maintenance

### Adding New Seeds
1. Create assembly source with exit stub:
   ```assembly
   # Your test code here
   
   # Exit via tohost (REQUIRED)
   lui  x5, 0x80001
   li   x6, 1
   sw   x6, 0(x5)
   j    .
   ```

2. Assemble and verify:
   ```bash
   riscv64-unknown-elf-as -march=rv32im -mabi=ilp32 -o test.o test.s
   riscv64-unknown-elf-objcopy -O binary test.o test.bin
   ./verify_exit_stubs.sh  # Verify exit stub present
   ```

3. Place in appropriate directory:
   - General fuzzing → `seeds/`
   - Bug-specific test → `test_seeds/`

### Fixing Corrupted Seeds
If a seed is missing its exit stub:
1. Disassemble to check: `riscv64-unknown-elf-objdump -D -b binary -m riscv:rv32 seed.bin`
2. Recreate source with exit stub
3. Reassemble and verify
4. Example: `quick_exit.bin` was fixed this way

## Documentation References

- **BUG_DETECTION_RESULTS.md**: Detailed bug analysis with security implications
- **test_seeds/README.md**: Test seed usage guide
- **verify_exit_stubs.sh**: Automated exit stub verification script

## Summary

✅ All 13 seeds verified with exit stubs
✅ Seeds organized by purpose (general vs. targeted)
✅ Automated verification script available
✅ Documentation complete for both directories
✅ Ready for fuzzing campaigns and bug validation
