    .section .text
    .globl _start
_start:
    # Shift operations seed
    addi x1, x0, 0xFF      # x1 = 255
    addi x2, x0, 4         # x2 = 4 (shift amount)
    
    sll  x3, x1, x2        # x3 = x1 << 4 = 4080
    srl  x4, x1, x2        # x4 = x1 >> 4 = 15
    
    # Arithmetic right shift
    addi x5, x0, -8        # x5 = -8 (0xFFFFFFF8)
    sra  x6, x5, x2        # x6 = x5 >>> 4 (sign extend)
    
    # Logical shifts
    slli x7, x1, 2         # x7 = x1 << 2
    srli x8, x1, 2         # x8 = x1 >> 2
    
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
