#!/usr/bin/env bash
# Run a single test case through the AFL harness with proper environment
# This ensures PROJECT_ROOT and TOHOST_ADDR are exported so both the harness
# and ISA mutator behave correctly.

set -euo pipefail

# Resolve project root (repo root is parent of tools/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default seed if not provided
SEED_PATH="${1:-$PROJECT_ROOT/seeds/firmware.bin}"

if [[ ! -f "$SEED_PATH" ]]; then
	echo "[ERROR] Seed file not found: $SEED_PATH" >&2
	echo "Usage: $0 [/absolute/or/relative/path/to/seed.bin]" >&2
	exit 2
fi

# Required env for config and exit-stub compatibility
export PROJECT_ROOT="$PROJECT_ROOT"
export TOHOST_ADDR="${TOHOST_ADDR:-0x80001000}"

# Toolchain and linker (used by Spike ELF builder)
export LD_BIN="${LD_BIN:-/opt/riscv/bin/riscv32-unknown-elf-ld}"
export LINKER_SCRIPT="${LINKER_SCRIPT:-$PROJECT_ROOT/tools/link.ld}"
export SPIKE_BIN="${SPIKE_BIN:-/opt/riscv/bin/spike}"
export SPIKE_ISA="${SPIKE_ISA:-rv32im}"
export OBJCOPY_BIN="${OBJCOPY_BIN:-/opt/riscv/bin/riscv32-unknown-elf-objcopy}"

# Make sure logs directories exist
mkdir -p "$PROJECT_ROOT/workdir/logs/crash" "$PROJECT_ROOT/workdir/traces"

# Build harness if missing
if [[ ! -x "$PROJECT_ROOT/afl/afl_picorv32" ]]; then
	echo "[BUILD] afl/afl_picorv32 not found. Building..."
	make -C "$PROJECT_ROOT/afl" build
fi

echo "[INFO] PROJECT_ROOT=$PROJECT_ROOT"
echo "[INFO] TOHOST_ADDR=$TOHOST_ADDR"
echo "[INFO] LD_BIN=$LD_BIN"
echo "[INFO] LINKER_SCRIPT=$LINKER_SCRIPT"
echo "[INFO] SPIKE_BIN=$SPIKE_BIN (ISA=$SPIKE_ISA)"
echo "[INFO] OBJCOPY_BIN=$OBJCOPY_BIN"
echo "[INFO] Seed: $SEED_PATH"
echo

set -x
"$PROJECT_ROOT/afl/afl_picorv32" "$SEED_PATH"
set +x

echo
echo "[INFO] Done. See logs at: $PROJECT_ROOT/workdir/logs/runtime.log"
