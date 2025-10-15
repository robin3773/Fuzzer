    .option arch, rv32im
    .text
    .globl _start
_start:
    li  t0, 10
    li  t1, 13
    add t2, t0, t1
    srai t3, t2, 1
    ebreak
