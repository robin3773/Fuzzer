#!/usr/bin/env python3
"""
Add exit stub to seed files that don't have it yet.
This appends the 5-instruction exit sequence to each .bin file in the seeds directory.
"""

import struct
import glob
import os
import sys
import shutil

# Exit stub configuration (must match afl/isa_mutator/include/fuzz/mutator/ExitStub.hpp)
TOHOST_ADDR = 0x80001000

# Calculate hi20/lo12 split (sign-correct for LUI/ADDI)
hi20 = (TOHOST_ADDR + 0x800) >> 12
lo12_raw = TOHOST_ADDR & 0xFFF
lo12 = lo12_raw if lo12_raw < 0x800 else lo12_raw - 0x1000

# Register allocation: t0 = x5, t1 = x6
T0 = 5
T1 = 6

# Build exit stub words
def encode_lui(rd, upper20):
    """Encode LUI instruction"""
    return (upper20 << 12) | (rd << 7) | 0x37

def encode_addi(rd, rs1, imm12):
    """Encode ADDI instruction"""
    uimm = imm12 & 0xFFF
    return (uimm << 20) | (rs1 << 15) | (rd << 7) | 0x13

def encode_sw(rs2, rs1, imm12):
    """Encode SW instruction"""
    uimm = imm12 & 0xFFF
    imm_lo = uimm & 0x1F
    imm_hi = (uimm >> 5) & 0x7F
    return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (0x2 << 12) | (imm_lo << 7) | 0x23

EBREAK = 0x00100073

# Generate exit stub
exit_stub = [
    encode_lui(T0, hi20),        # lui  t0, 0x80001
    encode_addi(T0, T0, lo12),   # addi t0, t0, 0
    encode_addi(T1, 0, 1),       # addi t1, x0, 1
    encode_sw(T1, T0, 0),        # sw   t1, 0(t0)
    EBREAK                        # ebreak
]

print(f"Exit stub configuration:")
print(f"  TOHOST_ADDR: 0x{TOHOST_ADDR:08x}")
print(f"  hi20: 0x{hi20:05x}")
print(f"  lo12: {lo12} (0x{lo12 & 0xFFF:03x})")
print(f"\nExit stub (5 words):")
for i, word in enumerate(exit_stub):
    print(f"  [{i}] 0x{word:08x}")
print()

def has_exit_stub(data):
    """Check if file already has exit stub at the end"""
    if len(data) < 20:
        return False
    
    # Check last 20 bytes
    offset = len(data) - 20
    for i in range(5):
        word_offset = offset + i * 4
        word = struct.unpack('<I', data[word_offset:word_offset+4])[0]
        if word != exit_stub[i]:
            return False
    return True

def add_exit_stub(binfile, backup=True):
    """Add exit stub to a binary file"""
    # Read original
    with open(binfile, 'rb') as f:
        data = bytearray(f.read())
    
    original_size = len(data)
    
    # Check if already has exit stub
    if has_exit_stub(data):
        print(f"  [{binfile}] Already has exit stub (size: {original_size} bytes) - SKIPPED")
        return False
    
    # Backup if requested
    if backup:
        backup_file = binfile + '.orig'
        if not os.path.exists(backup_file):
            shutil.copy2(binfile, backup_file)
            print(f"  [{binfile}] Created backup: {backup_file}")
    
    # Append exit stub (little-endian)
    for word in exit_stub:
        data.extend(struct.pack('<I', word))
    
    # Write back
    with open(binfile, 'wb') as f:
        f.write(data)
    
    new_size = len(data)
    print(f"  [{binfile}] Added exit stub: {original_size} -> {new_size} bytes (+{new_size - original_size})")
    return True

def main():
    # Determine seeds directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    seeds_dir = os.path.join(project_root, 'seeds')
    
    if len(sys.argv) > 1:
        seeds_dir = sys.argv[1]
    
    if not os.path.isdir(seeds_dir):
        print(f"ERROR: Seeds directory not found: {seeds_dir}")
        sys.exit(1)
    
    print(f"Processing seeds in: {seeds_dir}\n")
    
    # Find all .bin files
    bin_files = glob.glob(os.path.join(seeds_dir, '*.bin'))
    
    if not bin_files:
        print(f"WARNING: No .bin files found in {seeds_dir}")
        sys.exit(0)
    
    print(f"Found {len(bin_files)} seed file(s)\n")
    
    # Process each file
    modified_count = 0
    for binfile in sorted(bin_files):
        if add_exit_stub(binfile):
            modified_count += 1
    
    print(f"\n{'='*60}")
    print(f"Summary: Modified {modified_count}/{len(bin_files)} seed files")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()
