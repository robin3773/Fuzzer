# ======================================================================
# Root Makefile — PicoRV32 + Verilator + Firmware + AFL Fuzz wrappers
# ======================================================================

# ----------------------- Paths -----------------------
MODULE        ?= picorv32
FW_DIR        ?= firmware
FW_BIN        ?= $(FW_DIR)/build/firmware.bin
OBJ_DIR       ?= ./verilator/obj_dir
TRACE_DIR     ?= ./traces
WAVE_FILE     ?= $(TRACE_DIR)/waveform.vcd

AFL_DIR       ?= ./afl
AFL_MK        ?= $(AFL_DIR)/Makefile.fuzz
MUT_DIR       ?= $(AFL_DIR)/isa_mutator
MUT_SO        ?= $(MUT_DIR)/libisa_mutator.so

SIM_ARGS      ?=

.DEFAULT_GOAL := help

# ----------------------- Colors -----------------------
V ?= 1
Q := $(if $(filter $(V),0),@,)

YELLOW := \033[1;33m
GREEN  := \033[1;32m
BLUE   := \033[1;34m
RED    := \033[1;31m
CYAN   := \033[1;36m
RESET  := \033[0m

# ----------------------- Phony -----------------------
.PHONY: help info \
        sim waves verilate build lint \
        firmware firmware-clean firmware-rebuild \
        sim-clean sim-rebuild all-rebuild clean \
        mutator mutator-clean mutator-test \
        fuzz fuzz-dirs fuzz-build fuzz-run fuzz-clean fuzz-status

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
	@echo ""
	@echo "  Fuzzing (via $(AFL_MK)):"
	@echo "    $(GREEN)fuzz$(RESET)           Build mutator + harness + run AFL++"
	@echo "    $(GREEN)fuzz-build$(RESET)     Build everything, no run"
	@echo "    $(GREEN)fuzz-run$(RESET)       Run AFL++ if built"
	@echo "    $(GREEN)fuzz-clean$(RESET)     Remove corpora/seeds/traces"
	@echo ""
	@echo "  Mutator:"
	@echo "    $(GREEN)mutator$(RESET)        Build ISA-aware custom mutator (.so)"
	@echo "    $(GREEN)mutator-clean$(RESET)  Clean mutator build"
	@echo ""
	@echo "  Variables:"
	@echo "    V=$(V) (0=quiet,1=normal)"
	@echo ""

# ======================================================================
# Mutator wrappers
# ======================================================================
mutator:
	@echo "$(BLUE)[MUTATOR] Building ISA-aware mutator$(RESET)"
	$(Q)$(MAKE) -C $(MUT_DIR)
	@ls -lh $(MUT_SO) || true

mutator-test:
	$(Q)$(MAKE) -C $(MUT_DIR)
	$(Q)$(MAKE) -C $(MUT_DIR)/test run

mutator-clean:
	$(Q)$(MAKE) -C $(MUT_DIR) clean || true
	$(Q)$(MAKE) -C $(MUT_DIR)/test clean || true

# ======================================================================
# AFL++ fuzz wrappers (redirect to afl/Makefile)
# ======================================================================
fuzz:         ## build mutator + harness + run AFL
	$(Q)$(MAKE) -C $(AFL_DIR) fuzz

fuzz-dirs:
	$(Q)$(MAKE) -C $(AFL_DIR) dirs

fuzz-build:
	$(Q)$(MAKE) -C $(AFL_DIR) build

fuzz-run:
	$(Q)$(MAKE) -C $(AFL_DIR) fuzz

fuzz-clean:
	$(Q)$(MAKE) -C $(AFL_DIR) clean

fuzz-status:
	@echo "$(BLUE)[STATUS] AFL_DIR=$(AFL_DIR)$(RESET)"
	@echo "$(BLUE)[STATUS] MUTATOR=$(MUT_SO)$(RESET)"
	@ls -ld $(AFL_DIR)/../seeds $(AFL_DIR)/../corpora $(AFL_DIR)/../traces 2>/dev/null || true


clean: sim-clean firmware-clean mutator-clean fuzz-clean
	@rm -rfv workdir/*
	@echo "$(BLUE)[CLEAN] All cleanups done!$(RESET)"


# ======================================================================
# AFL/Makefile.fuzz — AFL++ + Verilator + RV32 Mutator Framework
# ======================================================================