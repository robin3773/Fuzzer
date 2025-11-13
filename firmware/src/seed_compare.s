    .section .text
    .globl _start
_start:
    # Comparison operations seed
    addi x1, x0, 10        # x1 = 10
    addi x2, x0, 20        # x2 = 20
    
    # Set less than
    slt  x3, x1, x2        # x3 = 1 (10 < 20)
    slt  x4, x2, x1        # x4 = 0 (20 not < 10)
    
    # Set less than unsigned
    addi x5, x0, -1        # x5 = -1 (0xFFFFFFFF)
    sltu x6, x1, x5        # x6 = 1 (10 < 0xFFFFFFFF unsigned)
    
    # Set less than immediate
    slti x7, x1, 15        # x7 = 1 (10 < 15)
    sltiu x8, x1, 5        # x8 = 0 (10 not < 5)
    
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
