    .section .text
    .globl _start
_start:
    # Jump and link seed
    addi x1, x0, 0         # x1 = 0 (counter)
    
    # JAL - jump and link
    jal  x2, func1         # Call func1, x2 = return address
    addi x1, x1, 100       # x1 += 100 (after return)
    
    # JALR - jump and link register
    lui  x3, %hi(func2)
    addi x3, x3, %lo(func2)
    jalr x4, x3, 0         # Call func2 via register
    
    # Done
    jal  x0, exit_program
    
func1:
    addi x1, x1, 10        # x1 += 10
    jalr x0, x2, 0         # Return using x2
    
func2:
    addi x1, x1, 20        # x1 += 20
    jalr x0, x4, 0         # Return using x4
    
exit_program:
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
