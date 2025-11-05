# ============================================================
# memory_config.mk — Central memory layout configuration
# ============================================================
# This is the SINGLE SOURCE OF TRUTH for all memory addresses.
# 
# Who reads this file:
#   - firmware/Makefile (passes to linker via -defsym)
#   - afl_harness/Makefile (passes to C++ via -D defines)
#   - verilator/Makefile (passes to testbench via -D defines)
#   - run.sh (can source or override these values)
#
# Usage in Makefiles:
#   include ../tools/memory_config.mk
# ============================================================

# ROM and program entry
PROGADDR_RESET   := 0x80000000
PROGADDR_IRQ     := 0x80000100
ROM_BASE         := 0x80000000
ROM_SIZE         := 0x00040000  # 256 KiB

# RAM layout
RAM_BASE         := 0x80040000
RAM_SIZE_BYTES   := 0x00040000  # 256 KiB
RAM_SIZE_ALIGNED := 0x00040000

# Stack configuration
STACK_ADDR       := 0x8007FFF0

# Host communication (for clean termination, e.g., Spike tohost flag)
TOHOST_ADDR      := 0x80001000

# Export all for sub-makes
export PROGADDR_RESET PROGADDR_IRQ ROM_BASE ROM_SIZE
export RAM_BASE RAM_SIZE_BYTES RAM_SIZE_ALIGNED STACK_ADDR TOHOST_ADDR
