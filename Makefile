# ======================================================================
# Root Makefile — PicoRV32 + Verilator + Firmware + AFL Fuzz wrappers
# Style: Hardcore Research (clean UX, helpful output, modular targets)
# Verbosity: V1 (balanced)
# ======================================================================

# -----------------------
# Project & Paths
# -----------------------
MODULE        ?= picorv32
FW_DIR        ?= firmware
FW_BIN        ?= $(FW_DIR)/build/firmware.bin
OBJ_DIR       ?= ./verilator/obj_dir
TRACE_DIR     ?= ./traces
WAVE_FILE     ?= $(TRACE_DIR)/waveform.vcd

AFL_DIR       ?= ./afl
AFL_MK        ?= $(AFL_DIR)/Makefile.fuzz
MUT_DIR       ?= $(AFL_DIR)/rv32_mutator
MUT_SO        ?= $(MUT_DIR)/librv32_mutator.so

# Pass plusargs to simulator; e.g.:
#   make sim SIM_ARGS="+print_every=100 +max_cycles=200000"
SIM_ARGS      ?=

.DEFAULT_GOAL := help

# -----------------------
# Colors & Verbosity
# -----------------------
V ?= 1
Q := $(if $(filter $(V),0),@,)

YELLOW := \033[1;33m
GREEN  := \033[1;32m
BLUE   := \033[1;34m
RED    := \033[1;31m
CYAN   := \033[1;36m
RESET  := \033[0m

# -----------------------
# Phony targets
# -----------------------
.PHONY: help info \
        sim waves verilate build lint \
        firmware firmware-clean firmware-rebuild \
        sim-clean sim-rebuild all-rebuild clean \
        mutator mutator-clean \
        mutator-test \
        fuzz fuzz-dirs fuzz-build fuzz-run fuzz-clean fuzz-status \


# ======================================================================
# Help
# ======================================================================
help:
	@echo ""
	@echo "  $(CYAN)PicoRV32 Project — Root Makefile$(RESET)"
	@echo ""
	@echo "  Primary targets:"
	@echo "    $(GREEN)sim$(RESET)            Build & run sim; generate $(WAVE_FILE)"
	@echo "    $(GREEN)waves$(RESET)          Open GTKWave for $(WAVE_FILE)"
	@echo "    $(GREEN)lint$(RESET)           Verilator lint"
	@echo "    $(GREEN)clean$(RESET)          Clean Verilator artifacts & waves"
	@echo ""
	@echo "  Firmware:"
	@echo "    $(GREEN)firmware$(RESET)       Build firmware inside $(FW_DIR)"
	@echo "    $(GREEN)firmware-clean$(RESET) Clean firmware in $(FW_DIR)"
	@echo "    $(GREEN)firmware-rebuild$(RESET) Clean + rebuild firmware"
	@echo ""
	@echo "  Rebuild flows:"
	@echo "    $(GREEN)sim-rebuild$(RESET)    Clean sim artifacts, rebuild & run sim"
	@echo "    $(GREEN)all-rebuild$(RESET)    Clean everything; rebuild firmware+sim+run"
	@echo ""
	@echo "  Fuzzing (wrappers; real logic in $(AFL_MK)):"
	@echo "    $(GREEN)fuzz$(RESET)           Build fuzz harness & mutator, run AFL"
	@echo "    $(GREEN)fuzz-clean$(RESET)     Clean AFL corpora/seeds/traces & mutator"
	@echo "    $(GREEN)fuzz-status$(RESET)    Show AFL dirs & mutator path"
	@echo ""
	@echo "  Mutator:"
	@echo "    $(GREEN)mutator$(RESET)        Build RV32 hybrid mutator (.so)"
	@echo "    $(GREEN)mutator-clean$(RESET)  Clean mutator build"
	@echo ""
	@echo "  Variables:"
	@echo "    V=$(V) (0=quiet,1=normal,2=verbose)"
	@echo "    SIM_ARGS='$(SIM_ARGS)'"
	@echo ""

info:
	@echo "$(BLUE)[INFO] MODULE=$(MODULE) OBJ_DIR=$(OBJ_DIR) FW_BIN=$(FW_BIN)$(RESET)"
	@echo "$(BLUE)[INFO] AFL_DIR=$(AFL_DIR) MUT_SO=$(MUT_SO)$(RESET)"

# ======================================================================
# Normal Simulation Flow (Verilator + Firmware)
# ======================================================================

# Run a simulation and generate/open the waveform
sim: $(WAVE_FILE)

waves: $(WAVE_FILE)
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@printf "  ║  WAVEFORM: $(YELLOW)%s$(RESET)\n" "$(WAVE_FILE)"
	@echo "  ║  OPEN WITH: gtkwave $(WAVE_FILE)"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)gtkwave $(WAVE_FILE)

# Generate waveform (ensure firmware is built first)
$(WAVE_FILE): $(OBJ_DIR)/V$(MODULE) $(FW_BIN) | $(TRACE_DIR)
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║       RUNNING SIMULATION TO GENERATE WAVEFORM.VCD            ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)./$(OBJ_DIR)/V$(MODULE) $(FW_BIN) +verilator+rand+reset+2 $(SIM_ARGS)
	$(Q)[ -f waveform.vcd ] && mv -f waveform.vcd $(WAVE_FILE) || true
	$(Q)touch $(WAVE_FILE)

$(TRACE_DIR):
	$(Q)mkdir -p $(TRACE_DIR)

# Verilator build
verilate: .stamp.verilate

build: $(OBJ_DIR)/V$(MODULE)

$(OBJ_DIR)/V$(MODULE): .stamp.verilate
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                BUILDING EXECUTABLE...                        ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)$(MAKE) -C $(OBJ_DIR) -f V$(MODULE).mk V$(MODULE)

.stamp.verilate: ./rtl/cpu/$(MODULE)/$(MODULE).v ./harness/tb_$(MODULE).cpp
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                    RUNNING VERILATOR...                      ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)mkdir -p $(OBJ_DIR)
	$(Q)verilator  -Wall --trace --x-assign unique --x-initial unique \
			--Wno-fatal --Wno-DECLFILENAME --Wno-UNUSEDSIGNAL --Wno-BLKSEQ --Wno-GENUNNAMED \
			--cc ./rtl/cpu/$(MODULE)/$(MODULE).v \
			--exe ../harness/tb_$(MODULE).cpp \
			-top-module $(MODULE) --Mdir ./verilator/obj_dir \
			-DRISCV_FORMAL
	$(Q)touch .stamp.verilate

# Lint
lint:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║             LINTING SYSTEMVERILOG SOURCE                     ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)verilator -Wall --lint-only --sv ./rtl/cpu/$(MODULE)/$(MODULE).v

# Firmware passthroughs
firmware:
	$(Q)$(MAKE) -C $(FW_DIR)

firmware-clean:
	$(Q)$(MAKE) -C $(FW_DIR) clean

firmware-rebuild: firmware-clean
	$(Q)$(MAKE) -C $(FW_DIR)

# Rebuild convenience
sim-clean:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                CLEAN VERILATOR OBJECTS & WAVES               ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)rm -rf $(OBJ_DIR) .stamp.verilate
	$(Q)rm -f  $(WAVE_FILE) ./waveform.vcd

sim-rebuild: sim-clean firmware
	$(Q)$(MAKE) verilate
	$(Q)$(MAKE) build
	$(Q)$(MAKE) sim

all-rebuild: firmware-clean sim-clean
	$(Q)$(MAKE) -C $(FW_DIR)
	$(Q)$(MAKE) verilate
	$(Q)$(MAKE) build
	$(Q)$(MAKE) sim

# Root clean
clean:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                      CLEANING UP...                          ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)rm -rvf $(OBJ_DIR) .stamp.verilate
	$(Q)rm -vf  $(WAVE_FILE) ./waveform.vcd

# ======================================================================
# Mutator (RV32 Hybrid) — convenience wrapper
# ======================================================================
mutator:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                BUILDING RV32 HYBRID MUTATOR                  ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	$(Q)$(MAKE) -C $(MUT_DIR)
	$(Q)ls -lh $(MUT_SO) || true

mutator-test:
	@$(MAKE) -C afl/rv32_mutator
	@$(MAKE) -C afl/rv32_mutator/test run

mutator-clean:
	$(Q)$(MAKE) -C $(MUT_DIR) clean || true
	$(Q)$(MAKE) -C afl/rv32_mutator/test clean || true

# ======================================================================
# AFL Fuzz — root wrappers (real logic inside afl/Makefile.fuzz)
# ======================================================================
fuzz:         ## build mutator + harness + run AFL (wrapper)
	$(Q)$(MAKE) -C $(AFL_DIR) -f $(AFL_MK) fuzz

fuzz-dirs:
	$(Q)$(MAKE) -C $(AFL_DIR) -f $(AFL_MK) dirs

fuzz-build:
	$(Q)$(MAKE) -C $(AFL_DIR) -f $(AFL_MK) build

fuzz-run:
	$(Q)$(MAKE) -C $(AFL_DIR) -f $(AFL_MK) fuzz

fuzz-clean:
	$(Q)$(MAKE) -C $(AFL_DIR) -f $(AFL_MK) clean

fuzz-status:
	@echo "$(BLUE)[STATUS] AFL_DIR=$(AFL_DIR)$(RESET)"
	@echo "$(BLUE)[STATUS] MUTATOR=$(MUT_SO)$(RESET)"
	@ls -ld $(AFL_DIR)/seeds $(AFL_DIR)/corpora $(AFL_DIR)/traces 2>/dev/null || true

