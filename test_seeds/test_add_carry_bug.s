# Test for ADD carry bug (bit 15→16)
# Bug: ADD ignores carry from lower 16 bits to upper 16 bits
# This test will expose the divergence between buggy DUT and correct Spike

.section .text
.globl _start
_start:
    # Load values that will trigger carry from bit 15 to bit 16
    lui  x1, 0x00018      # x1 = 0x00018000
    lui  x2, 0x00018      # x2 = 0x00018000
    
    # Perform ADD - should produce 0x00030000 but buggy DUT gives 0x00020000
    add  x3, x1, x2       # x3 = x1 + x2
    
    # Store result to trigger any memory-related bugs
    lui  x4, 0x80040      # x4 = 0x80040000 (RAM base)
    sw   x3, 0(x4)        # Store x3 to memory
    
    # Exit via tohost
    lui  x5, 0x80001      # x5 = 0x80001000
    li   x6, 1
    sw   x6, 0(x5)        # tohost = 1
    
    # Infinite loop (should not reach here)
1:  j 1b
