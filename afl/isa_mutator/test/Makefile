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
REPEAT  ?= 5
INPUT   ?= ../../../seeds/firmware.bin
OBJDUMP ?= riscv32-unknown-elf-objdump

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
	OBJDUMP=$(OBJDUMP) ./$(TEST_BIN) \
		--lib $(MUTATOR_SO) \
		$(if $(INPUT),--in $(INPUT),) \
		--seed $(SEED) \
		--repeat $(REPEAT)

clean:
	rm -f $(TEST_BIN)

.PHONY: all clean run
# ==========================================================