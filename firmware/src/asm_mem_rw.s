    .section .text
    .globl _start
_start:
    # set x1 = 8 (base pointer)
    addi x1, x0, 8

    # write value 0x42 to memory at offset 0(x1)
    addi x2, x0, 0x42
    sw   x2, 0(x1)

    # read back into x3
    lw   x3, 0(x1)

    # do something with value to avoid optimizer removal
    add  x4, x3, x1

    # store it to another offset
    sw   x4, 4(x1)

    # loop forever
1:  jal x0, 1b
