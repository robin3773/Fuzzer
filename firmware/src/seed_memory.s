    .section .text
    .globl _start
_start:
    # Memory operations seed
    lui  x1, 0x80001       # x1 = 0x80001000
    addi x1, x1, -2048     # x1 = 0x80000800
    
    # Store operations
    addi x2, x0, 0x42      # x2 = 0x42
    sb   x2, 0(x1)         # Store byte
    lui  x3, 1             # x3 = 0x1000
    addi x3, x3, 0x234     # x3 = 0x1234
    sh   x3, 4(x1)         # Store halfword
    lui  x4, 0xdead        # x4 = 0xdead0000
    addi x4, x4, -0x411    # x4 = 0xdeadbeef (0xbeef = -0x411 in 12-bit signed)
    sw   x4, 8(x1)         # Store word
    
    # Load operations
    lb   x5, 0(x1)         # Load byte
    lh   x6, 4(x1)         # Load halfword
    lw   x7, 8(x1)         # Load word
    
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
