#!/usr/bin/env python3
"""
Test exit stub encoding correctness by disassembling the 5-instruction sequence
"""

import subprocess
import tempfile
import os

# Expected exit stub for TOHOST_ADDR = 0x80001000
TOHOST_ADDR = 0x80001000

# Calculate hi20/lo12 split (sign-correct)
hi20 = (TOHOST_ADDR + 0x800) >> 12
lo12_raw = TOHOST_ADDR & 0xFFF
lo12 = lo12_raw if lo12_raw < 0x800 else lo12_raw - 0x1000

print(f"TOHOST_ADDR: 0x{TOHOST_ADDR:08x}")
print(f"  hi20: 0x{hi20:05x} ({hi20})")
print(f"  lo12: {lo12} (0x{lo12 & 0xFFF:03x})")
print()

# Expected encoding (little-endian)
# lui  t0, 0x80001  -> 0x800012b7
# addi t0, t0, 0    -> 0x00028293
# addi t1, x0, 1    -> 0x00100313
# sw   t1, 0(t0)    -> 0x0062a023  (NOTE: funct3=010 for SW)
# ebreak            -> 0x00100073

expected = [
    0x800012b7,  # lui  t0, 0x80001
    0x00028293,  # addi t0, t0, 0
    0x00100313,  # addi t1, x0, 1
    0x0062a023,  # sw   t1, 0(t0)
    0x00100073,  # ebreak
]

# Write binary to temp file
with tempfile.NamedTemporaryFile(mode='wb', suffix='.bin', delete=False) as f:
    for word in expected:
        f.write(word.to_bytes(4, 'little'))
    binfile = f.name

try:
    # Disassemble using objdump (try riscv-specific first, fallback to generic)
    objdump_cmd = None
    for cmd in ['riscv32-unknown-elf-objdump', 'riscv64-unknown-elf-objdump', 'objdump']:
        try:
            subprocess.run([cmd, '--version'], capture_output=True, check=True)
            objdump_cmd = cmd
            break
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
    
    if not objdump_cmd:
        print("ERROR: No suitable objdump found")
        exit(1)
    
    print(f"Using: {objdump_cmd}")
    print()
    
    result = subprocess.run([
        objdump_cmd,
        '-b', 'binary',
        '-m', 'riscv:rv32',
        '-D', binfile
    ], capture_output=True, text=True)
    
    if result.returncode != 0:
        print("ERROR: objdump failed")
        print(result.stderr)
        exit(1)
    
    print("Disassembly:")
    print("=" * 60)
    print(result.stdout)
    print("=" * 60)
    print()
    
    # Verify expected instructions (objdump uses pseudo-instructions)
    output = result.stdout
    checks = [
        ('lui', 't0,0x80001'),
        ('mv', 't0,t0'),       # addi t0,t0,0 shown as mv
        ('li', 't1,1'),        # addi t1,zero,1 shown as li
        ('sw', 't1,0(t0)'),    # sw instruction
        ('ebreak', ''),
    ]
    
    all_ok = True
    for i, (mnemonic, operands) in enumerate(checks):
        if mnemonic in output and (not operands or operands in output):
            print(f"✓ Instruction {i+1}: {mnemonic} {operands}")
        else:
            print(f"✗ Instruction {i+1}: {mnemonic} {operands} NOT FOUND")
            all_ok = False
    
    print()
    if all_ok:
        print("SUCCESS: Exit stub encoding is correct!")
    else:
        print("FAILURE: Exit stub encoding mismatch!")
        exit(1)

finally:
    os.unlink(binfile)
