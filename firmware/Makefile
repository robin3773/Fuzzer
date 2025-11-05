# ============================================================
#  RISC-V Firmware Makefile (for PicoRV32)
#  Builds every .S/.s source under src/ into its own .bin/.hex.
#  Toolchain: riscv32-unknown-elf-*
#  Author: Robin
# ============================================================

# ---- Include shared memory configuration ----
include ../tools/memory_config.mk

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

# Pass memory configuration to linker via -defsym
LDFLAGS   := -T../tools/link.ld \
             -defsym PROGADDR_RESET=$(PROGADDR_RESET) \
             -defsym PROGADDR_IRQ=$(PROGADDR_IRQ) \
             -defsym ROM_BASE=$(ROM_BASE) \
             -defsym ROM_SIZE=$(ROM_SIZE) \
             -defsym RAM_BASE=$(RAM_BASE) \
             -defsym RAM_SIZE=$(RAM_SIZE_ALIGNED) \
             -defsym STACK_ADDR=$(STACK_ADDR)

# ---- Sources and derived files ----
ASM_SRC   := $(wildcard $(SRC_DIR)/*.[sS])
OBJ       := $(patsubst $(SRC_DIR)/%,$(BUILD_DIR)/%,$(ASM_SRC:.S=.o))
OBJ       := $(OBJ:.s=.o)
ELF       := $(OBJ:.o=.elf)
BIN       := $(OBJ:.o=.bin)
HEX       := $(OBJ:.o=.hex)
DUMP      := $(OBJ:.o=.dump)

# ---- Targets ----
.PHONY: all clean dump dirs elf spike

all: dirs $(BIN) $(HEX)
	@echo "[✓] Built $(words $(ASM_SRC)) firmware images into $(BUILD_DIR)/"

# Generate ELF and BIN files (needed for Spike and harness)
elf: dirs $(ELF) $(BIN)
	@echo "[✓] Built $(words $(ELF)) ELF files and $(words $(BIN)) BIN files"

# Alias for spike
spike: elf

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
