# ===== Makefile for mutator_selftest (Clang version) =====

CXX      ?= clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
            -fno-omit-frame-pointer -g
LDFLAGS  ?= -ldl -lpthread

# Paths
ROOT_DIR    := ..
MUTATOR_SO  := $(ROOT_DIR)/libisa_mutator.so
INCLUDE_DIR := $(ROOT_DIR)/include
TEST_BIN    := mutator_selftest
TEST_SRC    := mutator_selftest.cpp

# Runtime knobs (now supported)
SEED    ?= 12345
REPEAT  ?= 1
INPUT   ?= $(abspath ../../../seeds/firmware.bin)
OBJDUMP ?= /opt/riscv/riscv32-unknown-elf/bin/objdump

# Environment variables for mutator (use absolute paths)
PROJECT_ROOT   ?= $(abspath $(ROOT_DIR)/../..)
MUTATOR_CONFIG ?= $(abspath $(ROOT_DIR)/config/mutator.default.yaml)
DEBUG          ?=

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(TEST_SRC) -o $(TEST_BIN) $(LDFLAGS)

run: $(TEST_BIN)
	@if [ ! -f $(MUTATOR_SO) ]; then \
		echo "[!] Mutator library not found: $(MUTATOR_SO)"; \
		echo "    Build it first: make -C $(ROOT_DIR)"; \
		exit 1; \
	fi
	@echo "[+] Running mutator self-test..."
	$(if $(DEBUG),DEBUG=$(DEBUG),) \
	PROJECT_ROOT=$(PROJECT_ROOT) \
	MUTATOR_CONFIG=$(MUTATOR_CONFIG) \
	OBJDUMP=$(OBJDUMP) \
	./$(TEST_BIN) \
		--lib $(abspath $(MUTATOR_SO)) \
		--in $(INPUT) \
		--seed $(SEED) \
		--repeat $(REPEAT)

debug: $(TEST_BIN)
	@$(MAKE) run DEBUG=1

clean:
	rm -f $(TEST_BIN)

.PHONY: all clean run debug
# ==========================================================