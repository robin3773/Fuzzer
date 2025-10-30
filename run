#!/usr/bin/env bash
# ==========================================================
# run.sh — AFL++ + ISA-aware mutator launcher (CLI-friendly)
# ==========================================================
# Examples:
#   ./run.sh --cores 8 --strategy RAW --timeout 500 --debug
#   ./run.sh -c 4 -s HYBRID -m none -t none --seeds ./seeds --out ./corpora
#   ./run.sh --no-build                 # run with existing binaries
#   ./run.sh --afl-args "-d -S test1"   # pass extra args to afl-fuzz
#
# Notes:
# - Verilator must be built deterministically (use --x-assign 0 --x-initial 0).
# - AFL_KEEP_ENV expects SPACE-separated names.
# ==========================================================

set -euo pipefail

# ---------- Defaults (match your repo) ----------
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"           # project root dir usually is script's dir
AFL_DIR="$PROJECT_ROOT/afl"
FUZZ_BIN="$AFL_DIR/afl_picorv32"
MUTATOR_SO="$AFL_DIR/isa_mutator/libisa_mutator.so"

STAMP="$(date +"%d-%b__%H-%M")"

SEEDS_DIR="$PROJECT_ROOT/seeds"
RUNS_DIR_DEFAULT="$PROJECT_ROOT/workdir"
RUNS_DIR="$RUNS_DIR_DEFAULT"     # may be overridden by --runs
RUN_DIR=""                        # computed after args
CORPORA_DIR=""                    # computed after args
LOG_FILE=""                       # computed after args

# Crash logs inside run dir
CRASH_DIR=""                      # computed after args

# ---------- Configurable params ----------
CORES="${CORES:-1}"
MEM_LIMIT="${MEM_LIMIT:-4G}"      # e.g., 4G or 'none'
TIMEOUT="${TIMEOUT:-100000}"       # ms or 'none'
STRATEGY="${RV32_STRATEGY:-HYBRID}"
ENABLE_C="${RV32_ENABLE_C:-1}"
DEBUG_MUTATOR="${DEBUG_MUTATOR:-0}"
MAX_CYCLES="${MAX_CYCLES:-50000}"

NO_BUILD=0
AFL_EXTRA_ARGS=""
AFL_DEBUG_FLAG=0

# ---------- Helpers ----------
usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  -c, --cores N            Number of AFL workers (default: ${CORES})
  -s, --strategy STR       RV32_STRATEGY = RAW | IR | HYBRID | AUTO (default: ${STRATEGY})
      --enable-c [0|1]     Enable compressed instruction path (default: ${ENABLE_C})
  -m, --mem LIMIT          AFL memory limit, e.g. 4G or 'none' (default: ${MEM_LIMIT})
  -t, --timeout MS         AFL timeout in ms or 'none' (default: ${TIMEOUT})
      --seeds DIR          Input seeds dir (default: ${SEEDS_DIR})
  -o, --out DIR            AFL corpora/output dir (if omitted, uses a timestamped run dir)
      --runs DIR           Base directory to store timestamped runs (default: ${RUNS_DIR_DEFAULT})
      --mutator PATH       Custom mutator .so (default: ${MUTATOR_SO})
      --bin PATH           Harness binary (default: ${FUZZ_BIN})
      --afl-args "ARGS"    Extra args passed to afl-fuzz (e.g. "-d -x dict")
      --no-build           Skip building mutator/harness (assume ready)
      --debug              Enable mutator debug (DEBUG_MUTATOR=1) and AFL_DEBUG=1
      --max-cycles N       Pass MAX_CYCLES to harness
      --golden MODE        GOLDEN_MODE = live | off | batch | replay (default: live)
      --spike PATH         Path to Spike binary (SPIKE_BIN)
      --pk PATH            Path to proxy kernel (PK_BIN)
      --objcopy PATH       Path to objcopy (OBJCOPY_BIN)
      --isa STR            Spike ISA string, e.g., rv32imc (SPIKE_ISA)
      --trace-mode on|off  Enable/disable harness trace writing (default: on)
      --exec-backend B     EXEC_BACKEND = verilator | fpga (default: verilator)
  -h, --help               Show this help and exit

Examples:
  $0 --cores 8 --strategy RAW --timeout 500 --debug
  $0 -c 4 -s HYBRID -m none -t none --seeds ./seeds
  $0 --no-build
  $0 --afl-args "-d -M main"
EOF
}

log() { printf "%s\n" "$@"; }

# ---------- Parse args ----------
while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--cores)        CORES="$2"; shift 2 ;;
    -s|--strategy)     STRATEGY="$2"; shift 2 ;;
    --enable-c)        ENABLE_C="$2"; shift 2 ;;
    -m|--mem)          MEM_LIMIT="$2"; shift 2 ;;
    -t|--timeout)      TIMEOUT="$2"; shift 2 ;;
    --seeds)           SEEDS_DIR="$(realpath -m "$2")"; shift 2 ;;
    --out)             CORPORA_DIR="$(realpath -m "$2")"; shift 2 ;;
    --runs)            RUNS_DIR="$(realpath -m "$2")"; shift 2 ;;
    --mutator)         MUTATOR_SO="$(realpath -m "$2")"; shift 2 ;;
    --bin)             FUZZ_BIN="$(realpath -m "$2")"; shift 2 ;;
    --afl-args)        AFL_EXTRA_ARGS="$2"; shift 2 ;;
    --no-build)        NO_BUILD=1; shift ;;
    --debug)           DEBUG_MUTATOR=1; AFL_DEBUG_FLAG=1; shift ;;
    --max-cycles)      MAX_CYCLES="$2"; shift 2 ;;
    --golden)          GOLDEN_MODE="$2"; shift 2 ;;
    --spike)           SPIKE_BIN="$(realpath -m "$2")"; shift 2 ;;
    --pk)              PK_BIN="$(realpath -m "$2")"; shift 2 ;;
    --objcopy)         OBJCOPY_BIN="$(realpath -m "$2")"; shift 2 ;;
    --isa)             SPIKE_ISA="$2"; shift 2 ;;
    --trace-mode)      TRACE_MODE="$2"; shift 2 ;;
    --exec-backend)    EXEC_BACKEND="$2"; shift 2 ;;
    -h|--help)         usage; exit 0 ;;
    *) log "[!] Unknown option: $1"; usage; exit 1 ;;
  esac
done

# ---------- Derived paths & run dir ----------
RUN_DIR="${RUNS_DIR}/${STAMP}"
mkdir -p "$RUN_DIR" "$SEEDS_DIR"

# If user didn't force an output dir, put corpora inside the run dir
if [[ -z "${CORPORA_DIR}" ]]; then
  CORPORA_DIR="${RUN_DIR}/corpora"
fi
mkdir -p "$CORPORA_DIR"

LOG_FILE="${RUN_DIR}/fuzz.log"
SPIKE_LOG_FILE="${RUN_DIR}/spike.log"
CRASH_DIR="${RUN_DIR}/logs/crash"
mkdir -p "$CRASH_DIR"
echo "[SETUP] Crash logs will go to: $CRASH_DIR"

# Optional traces directory inside run dir (for VCD/log triage)
TRACES_DIR="${RUN_DIR}/traces"
mkdir -p "$TRACES_DIR"

# ---------- Environment for AFL & mutator ----------
export AFL_SKIP_CPUFREQ=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$MUTATOR_SO"
export RV32_STRATEGY="$STRATEGY"
export RV32_ENABLE_C="$ENABLE_C"
export DEBUG_MUTATOR="$DEBUG_MUTATOR"
export MAX_CYCLES="$MAX_CYCLES"

# Propagate crash directory to harness using the name it expects
export CRASH_LOG_DIR="$CRASH_DIR"
export TRACE_DIR="$TRACES_DIR"
export SPIKE_LOG_FILE

# Golden/trace/backend defaults (honor pre-set env if provided)
export GOLDEN_MODE="${GOLDEN_MODE:-live}"
export EXEC_BACKEND="${EXEC_BACKEND:-verilator}"
export TRACE_MODE="${TRACE_MODE:-on}"

# Tooling defaults (if not provided via CLI/env)
: "${SPIKE_BIN:=/opt/riscv/bin/spike}"
: "${SPIKE_ISA:=rv32imc}"
: "${PK_BIN:=/opt/riscv/riscv32-unknown-elf/bin/pk}"
: "${OBJCOPY_BIN:=riscv32-unknown-elf-objcopy}"

# Warn if defaults not found; allow harness fallback by unsetting
if ! command -v "$SPIKE_BIN" >/dev/null 2>&1; then
  echo "[WARN] SPIKE_BIN not found at '$SPIKE_BIN' and not on PATH. Golden mode may be disabled."
fi
if ! command -v "$OBJCOPY_BIN" >/dev/null 2>&1; then
  echo "[WARN] OBJCOPY_BIN '$OBJCOPY_BIN' not found; harness will try 64-bit fallback."
  unset OBJCOPY_BIN
fi
if [[ -n "${PK_BIN:-}" && ! -x "$PK_BIN" ]]; then
  echo "[WARN] PK_BIN '$PK_BIN' not executable; Spike will run without pk."
  unset PK_BIN
fi

# Export detected/provided tooling
if [[ -n "${SPIKE_BIN:-}" ]];   then export SPIKE_BIN; fi
if [[ -n "${SPIKE_ISA:-}" ]];   then export SPIKE_ISA; fi
if [[ -n "${PK_BIN:-}" ]];      then export PK_BIN; fi
if [[ -n "${OBJCOPY_BIN:-}" ]]; then export OBJCOPY_BIN; fi

# Preserve these env vars in the target (SPACE-separated list)
export AFL_KEEP_ENV="CRASH_LOG_DIR TRACE_DIR MAX_CYCLES RV32_STRATEGY RV32_ENABLE_C GOLDEN_MODE EXEC_BACKEND TRACE_MODE SPIKE_BIN SPIKE_ISA PK_BIN OBJCOPY_BIN SPIKE_LOG_FILE"

# Optional AFL debug (very chatty)
if [[ "$AFL_DEBUG_FLAG" == "1" ]]; then
  export AFL_DEBUG=1
  export AFL_DEBUG_CHILD=1
fi

# Disable core dumps to avoid core_pattern warnings during AFL++ fuzzing runs
ulimit -c 0 || true

# ---------- Host core_pattern check (AFL compatibility) ----------
# If the kernel core_pattern pipes to an external utility (starts with '|'),
# AFL++ will abort unless AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 is set.
# We auto-enable the env var to keep runs smooth, and print a helpful note.
CORE_PATTERN="$(cat /proc/sys/kernel/core_pattern 2>/dev/null || true)"
if [[ "$CORE_PATTERN" == \|* && -z "${AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES:-}" ]]; then
  export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
  echo "[NOTE] core_pattern is piped to an external handler."
  echo "       Setting AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 to allow AFL to run."
  echo "       Recommended (requires sudo) to avoid delays:"
  echo "         echo core | sudo tee /proc/sys/kernel/core_pattern"
fi

# Preserve this env var in children as well (harmless if unset)
export AFL_KEEP_ENV="$AFL_KEEP_ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES"

# ---------- Pretty banner ----------
cat <<BANNER
==========================================================
  🧩 AFL++ + ISA Mutator Fuzzing Session
==========================================================
  Project Root : $PROJECT_ROOT
  Mutator SO   : $MUTATOR_SO
  Harness Bin  : $FUZZ_BIN
  Strategy     : $RV32_STRATEGY
  Enable C     : $ENABLE_C
  Debug        : $DEBUG_MUTATOR (AFL_DEBUG=$AFL_DEBUG_FLAG)
  Cores        : $CORES
  Timeout      : $TIMEOUT
  Mem Limit    : $MEM_LIMIT
  Seeds Dir    : $SEEDS_DIR
  Output Dir   : $CORPORA_DIR
  Run Log      : $LOG_FILE
  Spike Log    : $SPIKE_LOG_FILE
  Max Cycles   : $MAX_CYCLES
  Extra AFL    : ${AFL_EXTRA_ARGS:-<none>}
  Golden Mode  : $GOLDEN_MODE
  Exec Backend : $EXEC_BACKEND
  Trace Mode   : $TRACE_MODE
  Crash Logs   : $CRASH_DIR
  Trace Dir    : $TRACES_DIR
  SPIKE_BIN    : ${SPIKE_BIN:-<default>}
  SPIKE_ISA    : ${SPIKE_ISA:-<default>}
  PK_BIN       : ${PK_BIN:-<none>}
  OBJCOPY_BIN  : ${OBJCOPY_BIN:-<default>}
  AFL_DEBUG_FLAG : $AFL_DEBUG_FLAG
==========================================================
BANNER

# ---------- Preconditions ----------
if [[ "$NO_BUILD" -eq 1 ]]; then
  if [[ ! -x "$FUZZ_BIN" || ! -f "$MUTATOR_SO" ]]; then
    log "[!] --no-build requested but harness or mutator missing."
    log "    FUZZ_BIN:   $FUZZ_BIN"
    log "    MUTATOR_SO: $MUTATOR_SO"
    exit 1
  fi
fi

# ---------- Build (unless --no-build) ----------
if [[ "$NO_BUILD" -eq 0 ]]; then
  log "[BUILD] Building mutator + harness..."
  make -C "$AFL_DIR" build
fi

# ---------- Launch AFL++ ----------
log "[RUN] Starting AFL++..."
cd "$AFL_DIR"

# Compose AFL base args with clean handling of 'none'
AFL_BASE=(afl-fuzz -i "$SEEDS_DIR" -o "$CORPORA_DIR")
if [[ "$MEM_LIMIT" != "none" ]]; then AFL_BASE+=(-m "$MEM_LIMIT"); fi
if [[ "$TIMEOUT"   != "none" ]]; then AFL_BASE+=(-t "$TIMEOUT");   fi  # Add timeout argument if not 'none'

TARGET=(-- "$FUZZ_BIN" @@)

log "[INFO] MAX_CYCLES=${MAX_CYCLES} (passed to harness via env)"

mkdir -p "$(dirname "$LOG_FILE")"

if [[ "$CORES" == "1" ]]; then
  # shellcheck disable=SC2086
  "${AFL_BASE[@]}" $AFL_EXTRA_ARGS "${TARGET[@]}" 2>&1 | tee "$LOG_FILE"
else
  echo "[MASTER] Starting master instance..."
  # shellcheck disable=SC2086
  MAIN_LOG="${RUN_DIR}/main.log"
  "${AFL_BASE[@]}" -M main $AFL_EXTRA_ARGS "${TARGET[@]}" 2>&1 | tee "$MAIN_LOG" &
  sleep 2
  for i in $(seq 1 $((CORES - 1))); do
    echo "[SLAVE $i] Starting worker..."
    # shellcheck disable=SC2086
    SLAVE_LOG_FILE="${RUN_DIR}/fuzz_slave${i}.log"
    "${AFL_BASE[@]}" -S "slave$i" $AFL_EXTRA_ARGS "${TARGET[@]}" >> "$SLAVE_LOG_FILE" 2>&1 &
    sleep 2
  done
  wait
  # Concatenate logs into LOG_FILE for summary (handle case with no slaves)
  if ls "${RUN_DIR}/fuzz_slave"*.log >/dev/null 2>&1; then
    cat "$MAIN_LOG" "${RUN_DIR}/fuzz_slave"*.log > "$LOG_FILE"
  else
    cp "$MAIN_LOG" "$LOG_FILE"
  fi
fi

log "=========================================================="
log "  ✅ AFL++ session complete."
log "  Logs stored in: $LOG_FILE"
log "=========================================================="
# ---------- End of file ----------
