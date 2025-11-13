# Test Bug #6: SLL ignores bit 3 of shift amount
# SLL should shift by amount[4:0] (0-31), but buggy version uses amount[2:0] (0-7)
# So shift by 8 becomes shift by 0, shift by 9 becomes shift by 1, etc.

.section .text
.globl _start
_start:
    # Test 1: Shift by 8 (should shift 8 bits, buggy shifts 0 bits)
    li   x1, 0x000000FF   # FF in lowest byte
    li   x2, 8            # Shift amount
    sll  x3, x1, x2       # Should be 0x0000FF00, buggy gives 0x000000FF
    
    # Test 2: Shift by 16 (should shift 16 bits, buggy shifts 0 bits)  
    li   x4, 0x0000ABCD
    li   x5, 16
    sll  x6, x4, x5       # Should be 0xABCD0000, buggy gives 0x0000ABCD
    
    # Test 3: Shift by 9 (should shift 9 bits, buggy shifts 1 bit)
    li   x7, 0x00000001
    li   x8, 9
    sll  x9, x7, x8       # Should be 0x00000200, buggy gives 0x00000002
    
    # Test 4: SLLI immediate form - shift by 12
    li   x10, 0x00000123
    slli x11, x10, 12     # Should be 0x00123000, buggy gives 0x00000492 (shift by 4)
    
    # Test 5: Shift by 24 (should shift 24, buggy shifts 0)
    li   x12, 0x000000AA
    li   x13, 24
    sll  x14, x12, x13    # Should be 0xAA000000, buggy gives 0x000000AA

    # Exit via tohost
    lui  x15, 0x80001
    li   x1, 1
    sw   x1, 0(x15)
    
1:  j 1b
