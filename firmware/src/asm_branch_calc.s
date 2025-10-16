    .section .text
    .globl _start
_start:
    addi x1, x0, 5      # x1 = 5
    addi x2, x0, 3      # x2 = 3

    add  x3, x1, x2     # x3 = x1 + x2  (8)
    sub  x4, x1, x2     # x4 = x1 - x2  (2)
    sll  x5, x3, x2     # x5 = x3 << x2

    beq  x3, x4, equal  # unlikely, but branch exercised
    addi x6, x0, -1
    jal x0, continue

equal:
    addi x6, x0, 1

continue:
    # use registers
    add x7, x6, x5

    # loop
1:  jal x0, 1b
