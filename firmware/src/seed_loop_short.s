    .section .text
    .globl _start
_start:
    # Short counted loop seed (no infinite loops)
    addi x1, x0, 0         # x1 = 0 (counter)
    addi x2, x0, 5         # x2 = 5 (limit)
    
loop:
    addi x1, x1, 1         # x1++
    blt  x1, x2, loop      # Loop while x1 < 5
    
    # After loop, x1 = 5
    add  x3, x1, x1        # x3 = 10
    
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
