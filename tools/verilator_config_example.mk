# ============================================================
# verilator_config_example.mk — Example Verilator build integration
# ============================================================
# This shows how to integrate memory_config.mk with Verilator builds
# to ensure DUT and firmware use the same addresses.
# ============================================================

# Include shared memory configuration
include ../tools/memory_config.mk

# Verilator configuration
VERILATOR := verilator
VERILATOR_FLAGS := \
    --cc \
    --exe \
    --build \
    --trace \
    --top-module picorv32 \
    -Wall \
    -Wno-WIDTH \
    -Wno-UNUSED

# Pass memory configuration as C defines
VERILATOR_CFLAGS := \
    -DPROGADDR_RESET=$(PROGADDR_RESET) \
    -DPROGADDR_IRQ=$(PROGADDR_IRQ) \
    -DROM_BASE=$(ROM_BASE) \
    -DROM_SIZE=$(ROM_SIZE) \
    -DRAM_BASE=$(RAM_BASE) \
    -DRAM_SIZE_BYTES=$(RAM_SIZE_BYTES) \
    -DRAM_SIZE_ALIGNED=$(RAM_SIZE_ALIGNED) \
    -DSTACK_ADDR=$(STACK_ADDR)

# Example: Build Verilator model
verilate: picorv32.v testbench.cpp
	$(VERILATOR) $(VERILATOR_FLAGS) \
		-CFLAGS "$(VERILATOR_CFLAGS)" \
		picorv32.v testbench.cpp

# Example: In your C++ testbench, you can now use:
#   #ifndef PROGADDR_RESET
#   #define PROGADDR_RESET 0x80000000
#   #endif
#   
#   void reset_cpu() {
#       cpu->resetn = 0;
#       cpu->eval();
#       cpu->resetn = 1;
#       cpu->eval();
#       // PC should start at PROGADDR_RESET
#   }
