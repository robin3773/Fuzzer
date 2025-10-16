# ============================================================
#  RISC-V Firmware Makefile (for PicoRV32)
#  Builds every .S/.s source under src/ into its own .bin/.hex.
#  Toolchain: riscv32-unknown-elf-*
#  Author: Robin
# ============================================================

# ---- Directories ----
SRC_DIR    := src
BUILD_DIR  := build

# ---- Toolchain ----
RISCV_PREFIX ?= /opt/riscv/bin/riscv32-unknown-elf-
AS       := $(RISCV_PREFIX)as
LD       := $(RISCV_PREFIX)ld
OBJCOPY  := $(RISCV_PREFIX)objcopy
OBJDUMP  := $(RISCV_PREFIX)objdump

# ---- Flags ----
ARCH      := rv32im
ABI       := ilp32
ASFLAGS   := -march=$(ARCH) -mabi=$(ABI)
LDFLAGS   := -Ttext=0x00000000

# ---- Sources and derived files ----
ASM_SRC   := $(wildcard $(SRC_DIR)/*.[sS])
OBJ       := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(ASM_SRC:.S=.o))
OBJ       := $(OBJ:.s=.o)
ELF       := $(OBJ:.o=.elf)
BIN       := $(OBJ:.o=.bin)
HEX       := $(OBJ:.o=.hex)
DUMP      := $(OBJ:.o=.dump)

# ---- Targets ----
.PHONY: all clean dump dirs

all: dirs $(BIN) $(HEX)
	@echo "[✓] Built $(words $(ASM_SRC)) firmware images into $(BUILD_DIR)/"

dirs:
	@mkdir -p $(BUILD_DIR)

# Assemble and link each .S/.s -> .elf
$(BUILD_DIR)/%.elf: $(SRC_DIR)/%.S | dirs
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$*.o $<
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/$*.o

$(BUILD_DIR)/%.elf: $(SRC_DIR)/%.s | dirs
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$*.o $<
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/$*.o

# ELF -> binary
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

# ELF -> Verilog hex
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O verilog $< $@

# Optional disassembly
dump: $(DUMP)
	@echo "[i] Disassemblies written to $(BUILD_DIR)/"

$(BUILD_DIR)/%.dump: $(BUILD_DIR)/%.elf
	$(OBJDUMP) -d $< > $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Removed $(BUILD_DIR)/"
