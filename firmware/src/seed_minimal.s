    .section .text
    .globl _start
_start:
    # Minimal seed - just initialize and exit
    addi x1, x0, 1         # x1 = 1
    addi x2, x0, 2         # x2 = 2
    add  x3, x1, x2        # x3 = 3
    
    # Exit stub - write to tohost (0x80001000)
    lui  x5, 0x80001       # x5 = 0x80001000
    addi x5, x5, 0         # x5 = 0x80001000
    addi x6, x0, 1         # x6 = 1 (success code)
    sw   x6, 0(x5)         # store to tohost
    ebreak                 # halt
