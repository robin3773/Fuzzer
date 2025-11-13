# Test Bug #6: SLL ignores bit 3 of shift amount
# Simplified - avoid ADDI, use only LUI and immediate shifts

.section .text
.globl _start
_start:
    # Test 1: SLLI by 8 (should shift 8, buggy shifts 0)
    lui  x1, 0x00001      # x1 = 0x00001000
    slli x2, x1, 8        # x2 should be 0x00100000, buggy gives 0x00001000
    
    # Test 2: SLLI by 16 (should shift 16, buggy shifts 0)  
    lui  x3, 0x00002      # x3 = 0x00002000
    slli x4, x3, 16       # x4 should be 0x20000000, buggy gives 0x00002000
    
    # Test 3: SLLI by 12 (should shift 12, buggy shifts 4)
    lui  x5, 0x00003      # x5 = 0x00003000
    slli x6, x5, 12       # x6 should be 0x03000000, buggy gives 0x00030000
    
    # Test 4: SLLI by 24 (should shift 24, buggy shifts 0)
    lui  x7, 0x00004      # x7 = 0x00004000
    slli x8, x7, 24       # x8 should be 0x40000000, buggy gives 0x00004000
    
    # Test 5: SLLI by 20 (should shift 20, buggy shifts 4)
    lui  x9, 0x00005      # x9 = 0x00005000
    slli x10, x9, 20      # x10 should be 0x50000000, buggy gives 0x00050000

    # Exit via tohost
    lui  x11, 0x80001
    ori  x12, x0, 1
    sw   x12, 0(x11)
    
1:  j 1b
