# ==========================================================
# afl/Makefile — Build modular harness + RV32 custom mutator
# Compatible with run_fuzz.sh launcher
# ==========================================================

TOP_DIR     := $(abspath ..)
AFL_DIR     := $(abspath .)
MODULE      ?= picorv32

# RTL and object locations
RTL_FILE    := $(TOP_DIR)/rtl/cpu/$(MODULE)/$(MODULE).v
OBJ_DIR     := $(TOP_DIR)/verilator/obj_dir

# ----------------- HARNESS SOURCE STRUCTURE -----------------
# Modular harness directory (replaces single testbench)
HARNESS_DIR := $(TOP_DIR)/afl_harness
HARNESS_SRCS := \
  $(HARNESS_DIR)/harness_main.cpp \
  $(HARNESS_DIR)/cpu_$(MODULE).cpp
# (headers are automatically included)
# ------------------------------------------------------------

# Seed / corpus / trace directories
SEEDS_DIR   := $(TOP_DIR)/seeds
CORPORA_DIR := $(TOP_DIR)/workdir/corpora
TRACE_DIR   := $(TOP_DIR)/workdir/traces

# Mutator shared object
MUT_DIR     := $(AFL_DIR)/rv32_mutator
MUT_SO      := $(MUT_DIR)/librv32_mutator.so

# Output harness binary
FUZZ_EXE    := $(AFL_DIR)/afl_$(MODULE)

# Toolchain
CXXFLAGS    ?= -std=c++17 -O2 -g -fno-omit-frame-pointer
LDFLAGS     ?= -lpthread -latomic -ldl
TRACE       ?= 0

YELLOW := \033[1;33m
GREEN  := \033[1;32m
BLUE   := \033[1;34m
RESET  := \033[0m

.PHONY: all build check dirs verilate clean help

# ==========================================================
all: build
# ==========================================================

check:
	@which verilator >/dev/null || (echo "$(YELLOW)[ERROR] Verilator not found$(RESET)"; exit 1)
	@which afl-clang-fast++ >/dev/null || (echo "$(YELLOW)[ERROR] AFL++ not found$(RESET)"; exit 1)
	@echo "$(GREEN)[OK] Toolchain verified: Verilator + AFL++$(RESET)"

dirs:
	@mkdir -p $(SEEDS_DIR) $(CORPORA_DIR) $(TRACE_DIR)
	@echo "$(GREEN)[OK] Directories ready$(RESET)"

# ==========================================================
# VERILATE PHASE
# ==========================================================
verilate:
	@echo "$(BLUE)[VERILATE] Translating $(MODULE) (deterministic build)$(RESET)"
	verilator -Wall --x-assign 0 --x-initial 0 \
		-Wno-fatal -Wno-DECLFILENAME -Wno-UNUSEDSIGNAL \
		-Wno-BLKSEQ -Wno-GENUNNAMED \
		-DRISCV_FORMAL \
		--cc $(RTL_FILE) \
		--top-module $(MODULE) \
		--Mdir $(OBJ_DIR)

	@echo "$(GREEN)[OK] Verilated model generated$(RESET)"

# ==========================================================
# MUTATOR BUILD
# ==========================================================
$(MUT_SO):
	@echo "$(BLUE)[MUTATOR] Building RV32 hybrid mutator...$(RESET)"
	$(MAKE) -C $(MUT_DIR)
	@echo "$(GREEN)[OK] Built $(MUT_SO)$(RESET)"

# ==========================================================
# BUILD HARNESS (MODULAR)
# ==========================================================
build: check dirs verilate $(MUT_SO)
	@echo "$(BLUE)[BUILD] Compiling AFL harness...$(RESET)"
	# Rebuild the Verilator model with AFL and LLVM tools
	$(MAKE) -C $(OBJ_DIR) -f V$(MODULE).mk V$(MODULE)__ALL.a \
	    CC=afl-clang-fast++ CXX=afl-clang-fast++ \
	    AR=llvm-ar RANLIB=llvm-ranlib

	@if [ "$(TRACE)" = "1" ]; then \
	  echo "$(BLUE)[LINK] VCD trace ENABLED (slow)$(RESET)"; \
	  VCD_OBJ="/usr/share/verilator/include/verilated_vcd_c.cpp"; \
	else \
	  VCD_OBJ=""; \
	fi; \
	afl-clang-fast++ $(CXXFLAGS) -fuse-ld=lld \
		-I$(OBJ_DIR) \
		-I$(HARNESS_DIR) \
		-I/usr/share/verilator/include \
		-I/usr/share/verilator/include/vltstd \
		$(HARNESS_SRCS) \
		$(OBJ_DIR)/V$(MODULE)__ALL.a \
		/usr/share/verilator/include/verilated.cpp \
		/usr/share/verilator/include/verilated_threads.cpp \
		$$VCD_OBJ \
		-o $(FUZZ_EXE) \
		$(LDFLAGS)
	@echo "$(GREEN)[OK] Built harness binary: $(FUZZ_EXE)$(RESET)"

# ==========================================================
# CLEANUP
# ==========================================================
clean:
	@echo "$(YELLOW)[CLEAN] Removing build artifacts...$(RESET)"
	rm -rf $(OBJ_DIR) $(FUZZ_EXE)
	@$(MAKE) -C $(MUT_DIR) clean || true
	@echo "$(GREEN)[OK] Clean complete$(RESET)"

help:
	@echo ""
	@echo "$(BLUE)Targets:$(RESET)"
	@echo "  make all          - Build everything"
	@echo "  make verilate     - Run Verilator translation only"
	@echo "  make build        - Full build (Verilate + harness + mutator)"
	@echo "  make clean        - Remove all build outputs"
	@echo ""
	@echo "$(BLUE)Fuzzing:$(RESET)"
	@echo "  $(GREEN)FUZZ_EXE=$(FUZZ_EXE)$(RESET)"
	@echo "  $(GREEN)SEEDS_DIR=$(SEEDS_DIR)$(RESET)"
	@echo "  $(GREEN)CORPORA_DIR=$(CORPORA_DIR)$(RESET)"
	@echo "  $(GREEN)TRACE_DIR=$(TRACE_DIR)$(RESET)"
	@echo ""
	@echo "$(BLUE)Mutator:$(RESET)"
	@echo "  $(GREEN)MUT_SO=$(MUT_SO)$(RESET)"
	@echo ""
# ==========================================================