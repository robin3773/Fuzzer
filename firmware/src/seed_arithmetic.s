    .section .text
    .globl _start
_start:
    # Arithmetic operations seed
    addi x1, x0, 10        # x1 = 10
    addi x2, x0, 3         # x2 = 3
    
    add  x3, x1, x2        # x3 = 13
    sub  x4, x1, x2        # x4 = 7
    mul  x5, x1, x2        # x5 = 30 (if M extension)
    and  x6, x1, x2        # x6 = 2
    or   x7, x1, x2        # x7 = 11
    xor  x8, x1, x2        # x8 = 9
    
    # Exit stub
    lui  x5, 0x80001
    addi x5, x5, 0
    addi x6, x0, 1
    sw   x6, 0(x5)
    ebreak
