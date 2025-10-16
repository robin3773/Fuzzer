    .section .text
    .globl _start
_start:
    # NOP loop: addi x0,x0,0 is a NOP in RISC-V
    addi x1, x0, 0        # make sure registers are used
loop:
    addi x0, x0, 0        # NOP
    addi x0, x0, 0        # NOP
    addi x0, x0, 0        # NOP
    jal x0, loop          # jump back to loop (infinite loop)
