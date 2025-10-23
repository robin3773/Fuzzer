#!/usr/bin/env bash
# ==========================================================
# run_fuzz.sh — AFL++ + RV32 Mutator launcher (CLI-friendly)
# ==========================================================
# Examples:
#   ./run_fuzz.sh --cores 8 --strategy RAW --timeout 500 --debug
#   ./run_fuzz.sh -c 4 -s HYBRID -m none -t none --seeds ./seeds --out ./corpora
#   ./run_fuzz.sh --no-build                 # run with existing binaries
#   ./run_fuzz.sh --afl-args "-d -S test1"   # pass extra args to afl-fuzz
#
# Notes:
# - Verilator must be built deterministically (use --x-assign 0 --x-initial 0).
# - AFL_KEEP_ENV expects SPACE-separated names.
# ==========================================================

set -euo pipefail

# ---------- Defaults (match your repo) ----------
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
AFL_DIR="$PROJECT_ROOT/afl"
FUZZ_BIN="$AFL_DIR/afl_picorv32"
MUTATOR_SO="$AFL_DIR/rv32_mutator/librv32_mutator.so"

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
TIMEOUT="${TIMEOUT:-10000}"       # ms or 'none'
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
      --out DIR            AFL corpora/output dir (if omitted, uses a timestamped run dir)
      --runs DIR           Base directory to store timestamped runs (default: ${RUNS_DIR_DEFAULT})
      --mutator PATH       Custom mutator .so (default: ${MUTATOR_SO})
      --bin PATH           Harness binary (default: ${FUZZ_BIN})
      --afl-args "ARGS"    Extra args passed to afl-fuzz (e.g. "-d -x dict")
      --no-build           Skip building mutator/harness (assume ready)
      --debug              Enable mutator debug (DEBUG_MUTATOR=1) and AFL_DEBUG=1
      --max-cycles N       Pass MAX_CYCLES to harness
  -h, --help               Show this help and exit

Examples:
  $0 --cores 8 --strategy RAW --timeout 500 --debug
  $0 -c 4 -s HYBRID -m none -t none --seeds ./seeds
  $0 --no-build
  $0 --afl-args "-d -M main"
EOF
}

log() { echo -e "$@"; }

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
    --debug)           DEBUG_MUTATOR=0; AFL_DEBUG_FLAG=0; shift ;;
    --max-cycles)      MAX_CYCLES="$2"; shift 2 ;;
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
CRASH_DIR="${RUN_DIR}/logs/crash"
mkdir -p "$CRASH_DIR"
echo "[SETUP] Crash logs will go to: $CRASH_DIR"

# ---------- Environment for AFL & mutator ----------
export AFL_SKIP_CPUFREQ=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$MUTATOR_SO"
export RV32_STRATEGY="$STRATEGY"
export RV32_ENABLE_C="$ENABLE_C"
export DEBUG_MUTATOR="$DEBUG_MUTATOR"
export MAX_CYCLES="$MAX_CYCLES"

# Preserve these env vars in the target:
# NOTE: space-separated list (not comma-separated)
export AFL_KEEP_ENV="CRASH_DIR:MAX_CYCLES:RV32_STRATEGY:RV32_ENABLE_C"

# Optional AFL debug (very chatty)
if [[ "$AFL_DEBUG_FLAG" == "1" ]]; then
  export AFL_DEBUG=1
  export AFL_DEBUG_CHILD=1
fi

# Avoid core dump + core_pattern warning during fuzzing runs
ulimit -c 0 || true

# ---------- Pretty banner ----------
cat <<BANNER
==========================================================
  🧩 AFL++ + RV32 Mutator Fuzzing Session
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
  Max Cycles   : $MAX_CYCLES
  Extra AFL    : ${AFL_EXTRA_ARGS:-<none>}
==========================================================
BANNER

# ---------- Preconditions ----------
if [[ ! -x "$FUZZ_BIN" || ! -f "$MUTATOR_SO" ]] && [[ "$NO_BUILD" -eq 1 ]]; then
  log "[!] --no-build requested but harness or mutator missing."
  log "    FUZZ_BIN:   $FUZZ_BIN"
  log "    MUTATOR_SO: $MUTATOR_SO"
  exit 1
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
if [[ "$TIMEOUT"   != "none" ]]; then AFL_BASE+=(-t "$TIMEOUT");   fi # <-- THIS IS THE FIX

TARGET=(-- "$FUZZ_BIN" @@)

log "[INFO] MAX_CYCLES=${MAX_CYCLES} (passed to harness via env)"

mkdir -p "$(dirname "$LOG_FILE")"

if [[ "$CORES" == "1" ]]; then
  # shellcheck disable=SC2086
  "${AFL_BASE[@]}" $AFL_EXTRA_ARGS "${TARGET[@]}" 2>&1 | tee "$LOG_FILE"
else
  echo "[MASTER] Starting master instance..."
  # shellcheck disable=SC2086
  "${AFL_BASE[@]}" -M main $AFL_EXTRA_ARGS "${TARGET[@]}" 2>&1 | tee "$LOG_FILE" &
  sleep 2
  for i in $(seq 1 $((CORES - 1))); do
    echo "[SLAVE $i] Starting worker..."
    # shellcheck disable=SC2086
    "${AFL_BASE[@]}" -S "slave$i" $AFL_EXTRA_ARGS "${TARGET[@]}" >> "$LOG_FILE" 2>&1 &
    sleep 2
  done
  wait
fi

log "=========================================================="
log "  ✅ AFL++ session complete."
log "  Logs stored in: $LOG_FILE"
log "=========================================================="
# ---------- End of file ----------
