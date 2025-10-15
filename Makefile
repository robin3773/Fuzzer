# ===========================================
# Root Makefile for PicoRV32 + Verilator + FW
# ===========================================

MODULE      := picorv32
FW_DIR      := firmware
FW_BIN      := $(FW_DIR)/build/firmware.bin
OBJ_DIR     := ./verilator/obj_dir
TRACE_DIR   := ./traces
WAVE_FILE   := $(TRACE_DIR)/waveform.vcd

# Pass plusargs to simulator; e.g.:
#   make sim SIM_ARGS="+print_every=100 +max_cycles=200000"
SIM_ARGS    ?=

.DEFAULT_GOAL := sim

# -----------------------
# Phony targets
# -----------------------
.PHONY: sim verilate build waves lint clean \
        firmware firmware-clean firmware-rebuild \
        sim-clean sim-rebuild all-rebuild

# -----------------------
# Top-level flows
# -----------------------

# Run a simulation and generate/open the waveform
sim: $(WAVE_FILE)

waves: $(WAVE_FILE)
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║              WAVEFORM GENERATED: $(WAVE_FILE)                ║"
	@echo "  ║   OPEN WITH GTKWave: gtkwave $(WAVE_FILE)                    ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	gtkwave $(WAVE_FILE)

# Generate waveform (ensure firmware is built first)
$(WAVE_FILE): $(OBJ_DIR)/V$(MODULE) $(FW_BIN) | $(TRACE_DIR)
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║       RUNNING SIMULATION TO GENERATE WAVEFORM.VCD            ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@./$(OBJ_DIR)/V$(MODULE) $(FW_BIN) +verilator+rand+reset+2 $(SIM_ARGS)
	@# If harness wrote to CWD as waveform.vcd, move it to traces/
	@[ -f waveform.vcd ] && mv -f waveform.vcd $(WAVE_FILE) || true
	@# Ensure the wave file timestamp exists for Make deps
	@touch $(WAVE_FILE)

$(TRACE_DIR):
	@mkdir -p $(TRACE_DIR)

# -----------------------
# Verilator build
# -----------------------

verilate: .stamp.verilate

build: $(OBJ_DIR)/V$(MODULE)

$(OBJ_DIR)/V$(MODULE): .stamp.verilate
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                BUILDING EXECUTABLE...                        ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@$(MAKE) -C $(OBJ_DIR) -f V$(MODULE).mk V$(MODULE)

.stamp.verilate: ./rtl/cpu/$(MODULE)/$(MODULE).v ./harness/tb_$(MODULE).cpp
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                    RUNNING VERILATOR...                      ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@mkdir -p $(OBJ_DIR)
	verilator  -Wall --trace --x-assign unique --x-initial unique \
	           --Wno-fatal --Wno-DECLFILENAME --Wno-UNUSEDSIGNAL --Wno-BLKSEQ --Wno-GENUNNAMED \
	           --cc ./rtl/cpu/$(MODULE)/$(MODULE).v \
	           --exe ../harness/tb_$(MODULE).cpp \
	           -top-module $(MODULE) --Mdir $(OBJ_DIR)
	@touch .stamp.verilate

# -----------------------
# Lint
# -----------------------
lint:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║             LINTING SYSTEMVERILOG SOURCE                     ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	verilator -Wall --lint-only --sv ./rtl/cpu/$(MODULE)/$(MODULE).v

# -----------------------
# Firmware passthroughs
# -----------------------
firmware:
	@$(MAKE) -C $(FW_DIR)

firmware-clean:
	@$(MAKE) -C $(FW_DIR) clean

# Clean + rebuild firmware.bin (and .hex)
firmware-rebuild: firmware-clean
	@$(MAKE) -C $(FW_DIR)

# -----------------------
# Rebuild convenience
# -----------------------

# Clean only Verilator outputs & traces
sim-clean:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                CLEAN VERILATOR OBJECTS & WAVES               ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@rm -rf $(OBJ_DIR) .stamp.verilate
	@rm -f  $(WAVE_FILE) ./waveform.vcd

# Clean Verilator + rebuild firmware + rebuild simulator + run once
sim-rebuild: sim-clean firmware
	@$(MAKE) verilate
	@$(MAKE) build
	@$(MAKE) sim

# Clean EVERYTHING (firmware + verilator) and rebuild all, then run sim
all-rebuild: firmware-clean sim-clean
	@$(MAKE) -C $(FW_DIR)
	@$(MAKE) verilate
	@$(MAKE) build
	@$(MAKE) sim

# -----------------------
# Clean (root)
# -----------------------
clean:
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════════════╗"
	@echo "  ║                      CLEANING UP...                          ║"
	@echo "  ╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@rm -rf $(OBJ_DIR) .stamp.verilate
	@rm -f  $(WAVE_FILE) ./waveform.vcd
