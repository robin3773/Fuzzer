# ============================================================
# RISC-V Firmware Makefile (for PicoRV32)
# Uses riscv32-unknown-elf-* toolchain (pure assembler build)
# Author: Robin
# ============================================================

# ---- Directories ----
SRC_DIR   := src
BUILD_DIR := build
FW_NAME   := firmware

# ---- Toolchain ----
RISCV_PREFIX ?= /opt/riscv/bin/riscv32-unknown-elf-
AS      := $(RISCV_PREFIX)as
LD      := $(RISCV_PREFIX)ld
OBJCOPY := $(RISCV_PREFIX)objcopy
OBJDUMP := $(RISCV_PREFIX)objdump

# ---- Flags ----
ARCH     := rv32im
ABI      := ilp32
ASFLAGS  := -march=$(ARCH) -mabi=$(ABI)
LDFLAGS  := -Ttext=0x00000000

# ---- Files ----
ASM_SRC  := $(SRC_DIR)/$(FW_NAME).S
OBJ      := $(BUILD_DIR)/$(FW_NAME).o
ELF      := $(BUILD_DIR)/$(FW_NAME).elf
BIN      := $(BUILD_DIR)/$(FW_NAME).bin
HEX      := $(BUILD_DIR)/$(FW_NAME).hex
DUMP     := $(BUILD_DIR)/$(FW_NAME).dump

# ---- Targets ----
.PHONY: all clean dump

all: $(BIN) $(HEX)
	@echo "[✓] Firmware built successfully in $(BUILD_DIR)/"

# Assemble and link
$(ELF): $(ASM_SRC)
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -o $(OBJ) $(ASM_SRC)
	$(LD) $(LDFLAGS) -o $(ELF) $(OBJ)

# Convert ELF to binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Convert ELF to Verilog hex
$(HEX): $(ELF)
	$(OBJCOPY) -O verilog $< $@

# Optional: disassemble for inspection
dump: $(ELF)
	$(OBJDUMP) -d $(ELF) > $(DUMP)
	@echo "[i] Disassembly written to $(DUMP)"

# Cleanup
clean:
	rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Removed $(BUILD_DIR)/"
# ============================================================
# Verilator Makefile for PicoRV32 Simulation	