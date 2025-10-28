#!/usr/bin/env bash
# ==========================================================
# replay_golden.sh — Offline replay of a single seed with Spike сравнение
# Usage:
#   ./tools/replay_golden.sh <seed.bin>
# Optional env:
#   SPIKE_BIN, SPIKE_ISA=rv32imc, PK_BIN, OBJCOPY_BIN
#   GOLDEN_MODE=live|off (live by default)
#   XLEN=32 (passed through to harness)
# Output goes under workdir/replay-<timestamp>/
# ==========================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HARNESS="$ROOT/afl/afl_picorv32"
SEED="${1:-}"
if [[ -z "$SEED" || ! -f "$SEED" ]]; then
  echo "Usage: $0 <seed.bin>" >&2
  exit 2
fi

STAMP="replay-$(date +"%d-%b__%H-%M-%S")"
RUN_DIR="$ROOT/workdir/$STAMP"
CRASH_DIR="$RUN_DIR/logs/crash"
TRACES_DIR="$RUN_DIR/traces"
mkdir -p "$CRASH_DIR" "$TRACES_DIR"

# Defaults: live golden (per-instruction)
export GOLDEN_MODE="${GOLDEN_MODE:-live}"
export EXEC_BACKEND="${EXEC_BACKEND:-verilator}"
export TRACE_MODE="${TRACE_MODE:-on}"
export CRASH_LOG_DIR="$CRASH_DIR"
export TRACE_DIR="$TRACES_DIR"
# Pass optional paths if provided
[[ -n "${SPIKE_BIN:-}" ]] && export SPIKE_BIN
[[ -n "${SPIKE_ISA:-}" ]] && export SPIKE_ISA
[[ -n "${PK_BIN:-}"    ]] && export PK_BIN
[[ -n "${OBJCOPY_BIN:-}" ]] && export OBJCOPY_BIN

# Run harness on the single seed; stdout is tee'd into a run log
LOG_FILE="$RUN_DIR/replay.log"
"$HARNESS" "$SEED" 2>&1 | tee "$LOG_FILE"

echo "=========================================================="
echo "  Replay complete."
echo "  Run dir : $RUN_DIR"
echo "  Log     : $LOG_FILE"
echo "  Crashes : $CRASH_DIR"
echo "  Traces  : $TRACES_DIR"
echo "=========================================================="
