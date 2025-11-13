# Test for store byte mask bugs (SB/SH writing too many bytes)
# Bug 1: SH (store halfword) writes all 4 bytes instead of 2
# Bug 2: SB (store byte) writes all 4 bytes instead of 1

.section .text
.globl _start
_start:
    # Initialize memory with known pattern
    lui  x10, 0x80040     # x10 = 0x80040000 (RAM base)
    lui  x11, 0xAAAAB     # x11 = 0xAAAAB000
    addi x11, x11, -1366  # x11 = 0xAAAAAAAA (0xAAAAB000 + 0xFFFFFAAA= 0xAAAAAAAA)
    sw   x11, 0(x10)      # Write pattern to memory
    
    # Test SB (store byte) - should only write 1 byte at offset 0
    li   x12, 0x42        # x12 = 0x42
    sb   x12, 0(x10)      # Store byte at base
    # Buggy DUT: writes 0x42424242
    # Correct Spike: writes 0xAAAAAA42
    
    # Load word back and check
    lw   x13, 0(x10)      # x13 = memory[0x80040000]
    # x13 should be 0xAAAAAA42 but buggy DUT has 0x42424242
    
    # Test SH (store halfword) at offset 4 - should write 2 bytes
    lui  x14, 0xAAAAB     # Reset pattern
    addi x14, x14, -1366
    sw   x14, 4(x10)      # Write pattern to offset 4
    
    li   x15, 0x1234      # x15 = 0x1234
    sh   x15, 4(x10)      # Store halfword
    # Buggy DUT: writes 0x12341234
    # Correct Spike: writes 0xAAAA1234
    
    # Load and check
    lw   x16, 4(x10)      # x16 = memory[0x80040004]
    
    # Exit via tohost
    lui  x5, 0x80001
    li   x6, 1
    sw   x6, 0(x5)
    
1:  j 1b
