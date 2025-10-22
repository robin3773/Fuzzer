# ===== Makefile for mutator_selftest =====

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS  ?= -ldl

# Paths
MUTATOR_SO := ../librv32_mutator.so
TEST_BIN   := mutator_selftest
TEST_SRC   := mutator_selftest.cpp

# Env variables for tester
SEED   ?= 12345
REPEAT ?= 1
INPUT  ?=
OBJDUMP ?= riscv64-unknown-elf-objdump

all: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_SRC) -o $(TEST_BIN) $(LDFLAGS)

run: $(TEST_BIN)
	@if [ ! -f $(MUTATOR_SO) ]; then \
		echo "[!] Mutator library not found: $(MUTATOR_SO)"; \
		echo "    Build it with: make -C .."; \
		exit 1; \
	fi
	@echo "[+] Running self-test..."
	@OBJDUMP=$(OBJDUMP) ./$(TEST_BIN) \
		--lib $(MUTATOR_SO) \
		$(if $(INPUT),--in $(INPUT),) \
		--seed $(SEED) \
		--repeat $(REPEAT)

clean:
	rm -f $(TEST_BIN)

.PHONY: all clean run
# ======================================================================