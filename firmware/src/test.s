    .section .text
    .globl _start

_start:
    la a0, msg
    li a7, 4
    ecall
