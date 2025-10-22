# afl/rv32_mutator/test/Makefile
# Build & run the RV32 mutator self-test locally.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS  ?= -ldl

# Paths relative to this Makefile
MUTATOR_SO := ../librv32_mutator.so
TEST_BIN   := mutator_selftest
TEST_SRC   := mutator_selftest.cpp

# Default knobs (can be overridden on the command line)
SEED    ?= 12345
REPEAT  ?= 1
INPUT   ?=
OBJDUMP ?= riscv64-unknown-elf-objdump

.PHONY: all build run clean

all: build

build: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Run with optional args:
#   make run                   # uses default INPUT, SEED, REPEAT
#   make run INPUT=../../firmware/build/firmware.bin SEED=42 REPEAT=5
run: $(TEST_BIN) $(MUTATOR_SO)
	@echo "[+] Running $(TEST_BIN)"
	@OBJDUMP="$(OBJDUMP)" ./$(TEST_BIN) \
		--lib $(MUTATOR_SO) \
		$(if $(INPUT),--in $(INPUT),) \
		--seed $(SEED) \
		--repeat $(REPEAT)

clean:
	@rm -f $(TEST_BIN)
	@echo "[+] Cleaned up."