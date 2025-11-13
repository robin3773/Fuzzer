# Test Bug #7: OR always clears bit 31
# OR should compute A | B, but buggy version computes (A | B) & 0x7FFFFFFF

.section .text
.globl _start
_start:
    # Test 1: OR that should set bit 31
    lui  x1, 0x80000     # x1 = 0x80000000 (bit 31 set)
    lui  x2, 0x00001     # x2 = 0x00001000  
    or   x3, x1, x2      # x3 should be 0x80001000, buggy gives 0x00001000
    
    # Test 2: ORI with value that has bit 31
    lui  x4, 0x80000     # x4 = 0x80000000
    ori  x5, x4, 0xFF    # x5 should be 0x800000FF, buggy gives 0x000000FF
    
    # Test 3: OR two values both with bit 31 set
    lui  x6, 0xFFFFF     # x6 = 0xFFFFF000 (bit 31 set)
    lui  x7, 0x80000     # x7 = 0x80000000 (bit 31 set)
    or   x8, x6, x7      # x8 should be 0xFFFFF000, buggy gives 0x7FFFF000
    
    # Test 4: OR all ones pattern
    lui  x9, 0xFFFFF     # x9 = 0xFFFFF000
    ori  x10, x9, 0x7FF  # x10 should be 0xFFFFF7FF, buggy gives 0x7FFFF7FF

    # Exit via tohost  
    lui  x11, 0x80001
    ori  x12, x0, 1
    sw   x12, 0(x11)
    
1:  j 1b
