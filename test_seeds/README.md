# Test Seeds - Targeted Bug Detection

This directory contains test cases specifically designed to trigger intentionally injected bugs in the PicoRV32 CPU for differential testing validation.

## Purpose

These seeds are **not** for general fuzzing. They are targeted test cases that:
- Exercise specific CPU instructions with known bugs
- Validate differential checking detection capabilities
- Demonstrate security implications of hardware bugs
- Provide reproducible test cases for bug analysis

## File Organization

All test files follow the naming convention: `test_<bug_name>_bug.[s|o|bin]`

| File | Bug Tested | Instruction | Security Impact |
|------|------------|-------------|-----------------|
| `test_add_carry_bug.*` | Bug #3 | ADD/ADDI | 🔴 CRITICAL - Integer overflow |
| `test_beq_bug.*` | Bug #4 | BEQ | 🔴 CRITICAL - Auth bypass |
| `test_xor_bug.*` | Bug #5 | XOR | 🟡 HIGH - Crypto weakness |
| `test_xor_bug_simple.*` | Bug #5 | XOR (simplified) | 🟡 HIGH - Crypto weakness |
| `test_sll_bug.*` | Bug #6 | SLL | 🟡 HIGH - Bit manipulation |
| `test_sll_bug_simple.*` | Bug #6 | SLL (simplified) | 🟡 HIGH - Bit manipulation |
| `test_or_bug.*` | Bug #7 | OR | 🟡 HIGH - Sign confusion |
| `test_store_mask_bug.*` | Bug #1 & #2 | SB/SH | 🔴 CRITICAL - Memory corruption |

## Running Tests

To test a specific bug with differential checking:

```bash
cd /home/robin/HAVEN/Fuzz
GOLDEN_MODE=live ./afl/afl_picorv32 test_seeds/test_add_carry_bug.bin
```

Expected output: The harness should crash with `golden_divergence_*` error showing the bug was detected.

## Exit Stub Status

✅ **All test seeds have verified exit stubs**

Each test includes the standard exit pattern:
```assembly
lui  x5, 0x80001    # Load tohost address (0x80001000)
li   x6, 1          # Exit code
sw   x6, 0(x5)      # Signal completion
j    .              # Infinite loop
```

Verify with: `../verify_exit_stubs.sh`

## Test Results

See `../BUG_DETECTION_RESULTS.md` for:
- Detailed bug descriptions
- Security implications and attack scenarios
- Detection results and crash reports
- Real-world vulnerability comparisons

## Usage Guidelines

### DO:
✅ Use these seeds to validate differential checking
✅ Analyze crash reports to understand bug detection
✅ Study security implications for hardware verification
✅ Compare with regular seeds in `../seeds/` directory

### DON'T:
❌ Use these for general fuzzing campaigns (too specific)
❌ Expect these to find new bugs (they target known bugs)
❌ Remove exit stubs (required for proper termination)
❌ Mix with general fuzzing corpus (keep separate)

## Security Analysis

Each bug represents a realistic security vulnerability class:

**Memory Safety (Bugs #1, #2)**:
- Spatial safety violations
- Buffer overflows
- Adjacent data corruption

**Computation Integrity (Bug #3)**:
- Integer overflow
- Incorrect arithmetic
- Bounds check bypass

**Control Flow (Bug #4)**:
- Branch condition errors
- Authentication bypass
- Logic inversion

**Cryptographic (Bug #5)**:
- Bit manipulation errors
- Weak encryption output
- Predictable patterns

**Data Representation (Bugs #6, #7)**:
- Shift operation errors
- Sign bit corruption
- Pointer tag removal

## Additional Information

- **Created**: November 12, 2025
- **Framework**: Differential testing with Spike golden model
- **Target CPU**: PicoRV32 (buggy version with 7 injected bugs)
- **Detection Rate**: 71% (5/7 bugs detected)
- **Documentation**: ../BUG_DETECTION_RESULTS.md
