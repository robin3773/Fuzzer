# Test Bug #5: XOR always sets bit 0 to 1
# XOR should compute A ^ B, but buggy version computes (A ^ B) | 1

.section .text
.globl _start
_start:
    # Test 1: XOR that should produce even result (bit 0 = 0)
    li   x1, 0xAAAAAAAA  # ...1010
    li   x2, 0xAAAAAAAA  # ...1010
    xor  x3, x1, x2      # Should be 0x00000000, buggy DUT gives 0x00000001
    
    # Test 2: XOR with pattern that should clear bit 0
    li   x4, 0x12345678  # ...1000 (bit 0 = 0)
    li   x5, 0x11111110  # ...0000 (bit 0 = 0)  
    xor  x6, x4, x5      # Should have bit 0 = 0, buggy DUT sets bit 0 = 1
    
    # Test 3: XORI (immediate form)
    li   x7, 0xFFFFFFFC  # ...1100 (bit 0 = 0)
    xori x8, x7, 0x0FC   # 0xFC = ...11111100, result should have bit 0 = 0
    
    # Test 4: XOR producing known value
    li   x9, 0x55555554   # ...0100 (bit 0 = 0)
    li   x10, 0xAAAAAAAA  # ...1010
    xor  x11, x9, x10     # Should be 0xFFFFFFFE, buggy gives 0xFFFFFFFF

    # Exit via tohost
    lui  x12, 0x80001
    li   x13, 1
    sw   x13, 0(x12)
    
1:  j 1b
