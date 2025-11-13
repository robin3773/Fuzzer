    .section .text
    .globl _start
_start:
    # Branch testing seed
    addi x1, x0, 5         # x1 = 5
    addi x2, x0, 5         # x2 = 5
    
    # Test equal branch
    beq  x1, x2, equal_path
    addi x3, x0, 0         # Should not execute
    jal  x0, after_equal
    
equal_path:
    addi x3, x0, 1         # x3 = 1 (branch taken)
    
after_equal:
    # Test not equal branch
    addi x4, x0, 10
    bne  x1, x4, not_equal_path
    addi x5, x0, 0         # Should not execute
    jal  x0, done
    
not_equal_path:
    addi x5, x0, 1         # x5 = 1 (branch taken)
    
done:
    # Exit stub
    lui  x10, 0x80001
    addi x10, x10, 0
    addi x11, x0, 1
    sw   x11, 0(x10)
    ebreak
