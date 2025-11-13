# Test Bug #5: XOR always sets bit 0 to 1
# Simplified version - avoid ADDI to prevent ADD bug interference

.section .text
.globl _start
_start:
    # Test 1: XOR same value should give 0, but buggy gives 1
    lui  x1, 0x12345     # x1 = 0x12345000
    xor  x2, x1, x1      # x2 should be 0x00000000, buggy gives 0x00000001
    
    # Test 2: XOR with immediate (0) should preserve value with bit 0 set/cleared
    lui  x3, 0x55555     # x3 = 0x55555000 (bit 0 already clear)
    xori x4, x3, 0        # x4 should be 0x55555000, buggy gives 0x55555001
    
    # Test 3: Use LUI values that already have specific bit patterns
    lui  x5, 0xAAAAA     # x5 = 0xAAAAA000
    lui  x6, 0xAAAAA     # x6 = 0xAAAAA000  
    xor  x7, x5, x6       # x7 should be 0x00000000, buggy gives 0x00000001
    
    # Test 4: XOR different LUI values
    lui  x8, 0xFFFFF     # x8 = 0xFFFFF000
    lui  x9, 0x00000     # x9 = 0x00000000
    xor  x10, x8, x9      # x10 should be 0xFFFFF000, buggy gives 0xFFFFF001

    # Exit via tohost
    lui  x11, 0x80001
    ori  x12, x0, 1       # Use ORI instead of LI to avoid ADDI
    sw   x12, 0(x11)
    
1:  j 1b
