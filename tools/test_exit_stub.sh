#!/bin/bash
# Quick test to examine exit stub encoding

cd /home/robin/HAVEN/Fuzz

# Generate a mutated test case
export AFL_CUSTOM_MUTATOR_LIBRARY=afl/isa_mutator/libisa_mutator.so
export MUTATOR_CONFIG=afl/isa_mutator/config/mutator.default.yaml
export SCHEMA_DIR=schemas/riscv

# Create a temp file with the mutated output
TEMP_OUT=$(mktemp /tmp/mutated_XXXXXX.bin)

# Run harness once to trigger mutation
timeout 2s afl/afl_picorv32 seeds/asm_nop_loop.bin > /dev/null 2>&1

# Instead, let's examine one of the seeds after it goes through the system
echo "=== Original seed (asm_nop_loop.bin) ==="
hexdump -C seeds/asm_nop_loop.bin | head -20

echo ""
echo "=== Disassembly of original seed ==="
riscv32-unknown-elf-objdump -D -b binary -m riscv:rv32 --adjust-vma=0x80000000 seeds/asm_nop_loop.bin | head -30

echo ""
echo "=== Expected Exit Stub Pattern ==="
echo "For TOHOST_ADDR = 0x80001000:"
python3 << 'PYEOF'
TOHOST_ADDR = 0x80001000

# Split address
hi20 = (TOHOST_ADDR + 0x800) >> 12
base = hi20 << 12
lo12 = TOHOST_ADDR - base
if lo12 >= 2048:
    lo12 -= 4096

print(f"TOHOST_ADDR = 0x{TOHOST_ADDR:08x}")
print(f"HI20 = 0x{hi20:05x} ({hi20})")
print(f"LO12 = {lo12} (0x{lo12 & 0xfff:03x})")
print()

# Encode instructions
T0 = 5
T1 = 6

# LUI t0, hi20
lui = (hi20 << 12) | (T0 << 7) | 0x37
print(f"1. LUI  t0, 0x{hi20:05x}      → 0x{lui:08x}")

# ADDI t0, t0, lo12
addi_imm = lo12 & 0xfff
addi1 = (addi_imm << 20) | (T0 << 15) | (0x0 << 12) | (T0 << 7) | 0x13
print(f"2. ADDI t0, t0, {lo12:4d}   → 0x{addi1:08x}")

# ADDI t1, x0, 1
addi2 = (1 << 20) | (0 << 15) | (0x0 << 12) | (T1 << 7) | 0x13
print(f"3. ADDI t1, x0, 1      → 0x{addi2:08x}")

# SW t1, 0(t0)
sw = (0 << 25) | (T1 << 20) | (T0 << 15) | (0x2 << 12) | (0 << 7) | 0x23
print(f"4. SW   t1, 0(t0)      → 0x{sw:08x}")

# EBREAK
ebreak = 0x00100073
print(f"5. EBREAK              → 0x{ebreak:08x}")

print()
print("Little-endian byte sequence:")
for i, word in enumerate([lui, addi1, addi2, sw, ebreak], 1):
    bytes_le = [(word >> (8*j)) & 0xff for j in range(4)]
    print(f"{i}. {' '.join(f'{b:02x}' for b in bytes_le)}")
PYEOF

rm -f "$TEMP_OUT"
