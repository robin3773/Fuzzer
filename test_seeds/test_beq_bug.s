# Test Bug #4: BEQ inverted - branches when NOT equal
# BEQ should branch when operands are EQUAL, but buggy version branches when NOT equal

.section .text
.globl _start
_start:
    # Test 1: BEQ with equal operands (should branch)
    li   x1, 0x12345678
    li   x2, 0x12345678
    beq  x1, x2, equal_branch
    # If we reach here, BEQ failed (buggy behavior - didn't branch on equal)
    li   x3, 0xDEAD
    j    exit

equal_branch:
    # If we reach here, BEQ worked correctly (branched on equal)
    li   x3, 0xBEEF
    
    # Test 2: BEQ with different operands (should NOT branch)
    li   x4, 0x11111111
    li   x5, 0x22222222
    beq  x4, x5, bad_branch
    # If we reach here, BEQ correctly didn't branch on unequal
    li   x6, 0xC0DE
    j    exit
    
bad_branch:
    # If we reach here, BEQ incorrectly branched on unequal (buggy!)
    li   x6, 0xBAD0

exit:
    # Exit via tohost
    lui  x10, 0x80001
    li   x11, 1
    sw   x11, 0(x10)
    
1:  j 1b
