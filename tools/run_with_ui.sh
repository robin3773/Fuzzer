#!/bin/bash
# Run AFL++ fuzzer with live TUI interface visible

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Create run directory
TIMESTAMP=$(date +"%d-%b__%H-%M")
RUN_DIR="workdir/${TIMESTAMP}"
mkdir -p "$RUN_DIR"/{corpora,logs/crash,traces}

echo "Starting AFL++ with live UI..."
echo "Run directory: $RUN_DIR"
echo ""

# Set all required environment variables
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$PROJECT_ROOT/afl/isa_mutator/libisa_mutator.so"
export MUTATOR_CONFIG="$PROJECT_ROOT/afl/isa_mutator/config/mutator.default.yaml"
export SCHEMA_DIR="$PROJECT_ROOT/schemas"
export RV32_STRATEGY="${RV32_STRATEGY:-HYBRID}"
export RV32_ENABLE_C="${RV32_ENABLE_C:-1}"
export DEBUG_MUTATOR="${DEBUG_MUTATOR:-0}"
export MAX_CYCLES="${MAX_CYCLES:-50000}"

# Harness configuration
export CRASH_LOG_DIR="$RUN_DIR/logs/crash"
export TRACE_DIR="$RUN_DIR/traces"
export SPIKE_LOG_FILE="$RUN_DIR/logs/spike.log"
export GOLDEN_MODE="${GOLDEN_MODE:-live}"
export EXEC_BACKEND="${EXEC_BACKEND:-verilator}"
export TRACE_MODE="${TRACE_MODE:-on}"

# CPU parameters
export RAM_BASE="${RAM_BASE:-0x80040000}"
export RAM_SIZE="${RAM_SIZE:-0x00040000}"
export PROGADDR_RESET="${PROGADDR_RESET:-0x80000000}"
export PROGADDR_IRQ="${PROGADDR_IRQ:-0x80000100}"
export STACK_ADDR="${STACK_ADDR:-0x8007FFF0}"
export TOHOST_ADDR="${TOHOST_ADDR:-0x80001000}"
export PC_STAGNATION_LIMIT="${PC_STAGNATION_LIMIT:-512}"
export MAX_PROGRAM_WORDS="${MAX_PROGRAM_WORDS:-256}"
export STOP_ON_SPIKE_DONE="${STOP_ON_SPIKE_DONE:-1}"

# Spike toolchain (optional for golden mode)
export SPIKE_BIN="${SPIKE_BIN:-/opt/riscv/bin/spike}"
export SPIKE_ISA="${SPIKE_ISA:-rv32im}"
export OBJCOPY_BIN="${OBJCOPY_BIN:-/opt/riscv/bin/riscv32-unknown-elf-objcopy}"
export LD_BIN="${LD_BIN:-/opt/riscv/bin/riscv32-unknown-elf-ld}"
export LINKER_SCRIPT="$PROJECT_ROOT/tools/link.ld"

# Build if needed
if [[ ! -f "afl/afl_picorv32" ]] || [[ ! -f "afl/isa_mutator/libisa_mutator.so" ]]; then
    echo "[BUILD] Building fuzzer components..."
    make -C afl
fi

# Run AFL++ with live TUI
# The UI will be displayed directly in your terminal
echo ""
echo "Starting AFL++..."
echo "Press Ctrl+C to stop"
echo ""
sleep 2

afl-fuzz \
    -i seeds \
    -o "$RUN_DIR/corpora" \
    -m 4G \
    -t 100000 \
    -- \
    afl/afl_picorv32 @@
