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
export PROJECT_ROOT  # Export so ISA mutator can find schemas
AFL_DIR="$PROJECT_ROOT/afl"
FUZZ_BIN="$AFL_DIR/afl_picorv32"
MUTATOR_SO="$AFL_DIR/isa_mutator/libisa_mutator.so"
MUTATOR_CONFIG="$PROJECT_ROOT/afl/isa_mutator/config/mutator.default.yaml"

SEEDS_DIR="$PROJECT_ROOT/seeds"
RUN_DIR="$PROJECT_ROOT/workdir"   # Fixed directory name, no timestamp
CORPORA_DIR=""                    # computed after args
LOG_FILE=""                       # computed after args

# Crash logs inside run dir
CRASH_DIR=""                      # computed after args

# Backup flag
BACKUP_WORKDIR=0

# ---------- Configurable params ----------
CORES="${CORES:-1}"
MEM_LIMIT="${MEM_LIMIT:-4G}"        # e.g., 4G or 'none'
TIMEOUT="${TIMEOUT:-100000}"       # ms or 'none' (100 seconds default - needed for Verilator init + Spike execution)
DEBUG="${DEBUG:-0}"                # ISA mutator & harness debug logging
MAX_CYCLES="${MAX_CYCLES:-50000}"
UI_MODE="${UI_MODE:-auto}"         # auto | on | off (AFL++ status screen)
ENABLE_COVERAGE="${ENABLE_COVERAGE:-0}"  # Verilator coverage (temporarily disabled - API update needed)

NO_BUILD=0
AFL_EXTRA_ARGS=""
AFL_DEBUG="${AFL_DEBUG:-0}"        # AFL++ debug output (separate from DEBUG)

# ---------- Helpers ----------
usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  -c, --cores N               Number of AFL workers (default: ${CORES})
  -m, --mem LIMIT             AFL memory limit, e.g. 4G or 'none' (default: ${MEM_LIMIT})
  -t, --timeout MS            AFL timeout in ms or 'none' (default: ${TIMEOUT}, auto 180s for golden mode)
      --seeds DIR             Input seeds dir (default: ${SEEDS_DIR})
  -o, --out DIR               AFL corpora/output dir (if omitted, uses workdir/corpora)
      --mutator PATH          Custom mutator .so (default: ${MUTATOR_SO})
      --mutator-config PATH   Path to mutator config YAML file (default: afl/isa_mutator/config/mutator.default.yaml)
      --bin PATH              Harness binary (default: ${FUZZ_BIN})
      --afl-args "ARGS"       Extra args passed to afl-fuzz (e.g. "-d -x dict")
      --no-build              Skip building mutator/harness (assume ready)
  -bk, --backup               Backup current workdir before starting new run (creates workdir.backup.<timestamp>)
      --debug                 Enable ISA mutator & harness debug logging (DEBUG=1)
      --afl-debug             Enable AFL++ debug output (AFL_DEBUG=1) - very verbose
      --ui-mode MODE          AFL++ UI mode: auto (default) | on | off
      --max-cycles N          Pass MAX_CYCLES to harness
      --coverage on|off       Enable/disable Verilator coverage (default: on)
      --golden MODE           GOLDEN_MODE = live | off | batch | replay (default: live)
      --spike PATH            Path to Spike binary (SPIKE_BIN)
      --objcopy PATH          Path to objcopy (OBJCOPY_BIN)
      --isa STR               Spike ISA string, e.g., rv32imc (SPIKE_ISA)
      --trace-mode on|off     Enable/disable harness trace writing (default: on)
      --exec-backend B        EXEC_BACKEND = verilator | fpga (default: verilator)
  -h, --help                  Show this help and exit

Examples:
  $0 --cores 8 --timeout 500 --debug          # Enable mutator/harness debug only
  $0 -c 4 -m none -t none --seeds ./seeds
  $0 --no-build
  $0 -bk --cores 8                             # Backup workdir before starting
  $0 --ui-mode off                             # Disable AFL++ TUI (for logging/scripts)
  $0 --ui-mode on                              # Force AFL++ TUI display
  $0 --afl-debug                               # Enable AFL++ verbose debug output
  $0 --debug --afl-debug                       # Enable both (very chatty!)
  $0 --afl-args "-d -M main"
EOF
}

log() { printf "%s\n" "$@"; }

# ---------- Parse args ----------
while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--cores)        CORES="$2"; shift 2 ;;
    -m|--mem)          MEM_LIMIT="$2"; shift 2 ;;
    -t|--timeout)      TIMEOUT="$2"; shift 2 ;;
    --seeds)           SEEDS_DIR="$(realpath -m "$2")"; shift 2 ;;
    --out)             CORPORA_DIR="$(realpath -m "$2")"; shift 2 ;;
    --mutator)         MUTATOR_SO="$(realpath -m "$2")"; shift 2 ;;
    --mutator-config)  MUTATOR_CONFIG="$(realpath -m "$2")"; shift 2 ;;
    --bin)             FUZZ_BIN="$(realpath -m "$2")"; shift 2 ;;
    --afl-args)        AFL_EXTRA_ARGS="$2"; shift 2 ;;
    --no-build)        NO_BUILD=1; shift ;;
    -bk|--backup)      BACKUP_WORKDIR=1; shift ;;
    --debug)           DEBUG=1; shift ;;
    --afl-debug)       AFL_DEBUG=1; shift ;;
    --ui-mode)         UI_MODE="$2"; shift 2 ;;
    --max-cycles)      MAX_CYCLES="$2"; shift 2 ;;
    --coverage)        
      case "$2" in
        on|1)  ENABLE_COVERAGE=1 ;;
        off|0) ENABLE_COVERAGE=0 ;;
        *) log "[!] Invalid --coverage value: $2 (use 'on' or 'off')"; exit 1 ;;
      esac
      shift 2 ;;
  # Removed: GOLDEN_MODE, SPIKE_BIN, SPIKE_ISA, TRACE_MODE, EXEC_BACKEND are now only set in harness.conf
    -h|--help)         usage; exit 0 ;;
    *) log "[!] Unknown option: $1"; usage; exit 1 ;;
  esac
done

# ---------- Validate UI mode ----------
case "$UI_MODE" in
  auto|on|off) ;; # Valid values
  *)
    log "[!] ERROR: Invalid --ui-mode value: '$UI_MODE'" >&2
    log "    Valid options: auto, on, off" >&2
    exit 1
    ;;
esac

# ---------- Derived paths & run dir ----------
# Load memory configuration from centralized config file
MEMORY_CONFIG="$PROJECT_ROOT/tools/memory_config.mk"
if [[ ! -f "$MEMORY_CONFIG" ]]; then
  log "[!] Memory config file not found: $MEMORY_CONFIG" >&2
  exit 1
fi

# Source memory addresses from the centralized config
# Convert Make syntax (VAR := value) to bash exports
eval "$(grep -E '^[A-Z_]+\s*:=\s*0x[0-9A-Fa-f]+' "$MEMORY_CONFIG" | sed 's/\s*:=\s*/=/' | sed 's/^/export /')"

# Validate that critical values were loaded
if [[ -z "${PROGADDR_RESET:-}" || -z "${RAM_BASE:-}" || -z "${STACK_ADDR:-}" || -z "${TOHOST_ADDR:-}" ]]; then
  log "[!] Failed to load memory configuration from $MEMORY_CONFIG" >&2
  log "[!] PROGADDR_RESET=${PROGADDR_RESET:-<unset>}" >&2
  log "[!] RAM_BASE=${RAM_BASE:-<unset>}" >&2
  log "[!] STACK_ADDR=${STACK_ADDR:-<unset>}" >&2
  log "[!] TOHOST_ADDR=${TOHOST_ADDR:-<unset>}" >&2
  exit 1
fi

# Set aliases for compatibility
export STACKADDR="$STACK_ADDR"
export RAM_SIZE="$RAM_SIZE_ALIGNED"  # Alias for compatibility
# TOHOST_ADDR is already exported by eval above

# Calculate RAM_SIZE_BYTES decimal value for display
RAM_SIZE_BYTES_DEC=$((RAM_SIZE_BYTES))

# ---------- Backup existing workdir if requested ----------
if [[ "$BACKUP_WORKDIR" -eq 1 && -d "$RUN_DIR" ]]; then
  BACKUP_STAMP="$(date +"%Y%m%d_%H%M%S")"
  BACKUP_DIR="${RUN_DIR}.backup.${BACKUP_STAMP}"
  log "[BACKUP] Creating backup of existing workdir..."
  log "         From: $RUN_DIR"
  log "         To:   $BACKUP_DIR"
  
  if mv "$RUN_DIR" "$BACKUP_DIR"; then
    log "[BACKUP] ✓ Backup successful"
  else
    log "[!] ERROR: Failed to backup workdir" >&2
    exit 1
  fi
fi

# ---------- Create workdir structure ----------
mkdir -p "$RUN_DIR" "$SEEDS_DIR"

# If user didn't force an output dir, put corpora inside workdir
if [[ -z "${CORPORA_DIR}" ]]; then
  CORPORA_DIR="${RUN_DIR}/corpora"
fi
mkdir -p "$CORPORA_DIR"

LOG_FILE="${RUN_DIR}/fuzz.log"

# Runtime log directory for unified logging (replaces old harness.log)
# All mutator and harness logs go to: ${LOGS_DIR}/runtime.log
LOGS_DIR="${RUN_DIR}/logs"
mkdir -p "$LOGS_DIR"

SPIKE_LOG_FILE="${LOGS_DIR}/spike.log"
CRASH_DIR="${LOGS_DIR}/crash"
mkdir -p "$CRASH_DIR"
echo "[SETUP] Crash logs will go to: $CRASH_DIR"

# ---------- Load fuzzer environment configuration ----------
FUZZER_ENV="${PROJECT_ROOT}/fuzzer.env"
if [[ -f "$FUZZER_ENV" ]]; then
  log "[CONFIG] Loading environment from: $FUZZER_ENV"
  # Source the config file to load default env vars (won't override existing ones)
  set -a  # Auto-export all variables
  source "$FUZZER_ENV"
  set +a
else
  log "[WARN] fuzzer.env not found at: $FUZZER_ENV"
  log "       Using command-line defaults only"
fi

# ---------- Environment for AFL & mutator ----------
# Note: Defaults are loaded from fuzzer.env; these exports allow CLI overrides
export AFL_SKIP_CPUFREQ=1
export AFL_CUSTOM_MUTATOR_LIBRARY="$MUTATOR_SO"
export DEBUG="${DEBUG:-0}"
export MAX_CYCLES="${MAX_CYCLES:-50000}"

# AFL fork server initialization timeout (ms)
# Increase this if harness needs time to initialize Verilator/Spike before __AFL_LOOP
export AFL_FORKSRV_INIT_TMOUT=10000  # 10 seconds for Verilator init

# Tell AFL++ to preserve these environment variables across the fork server
# This is CRITICAL - without this, the harness won't see PROJECT_ROOT, TOHOST_ADDR, etc.
# Format: space-separated list of variable names (AFL_KEEP_ENV expects SPACE-separated, not colons)
export AFL_KEEP_ENV="PROJECT_ROOT TOHOST_ADDR PROGADDR_RESET RAM_BASE STACK_ADDR STACKADDR RAM_SIZE DEBUG MAX_CYCLES MUTATOR_CONFIG SCHEMA_DIR CRASH_LOG_DIR SPIKE_LOG_FILE GOLDEN_MODE TRACE_MODE EXEC_BACKEND SPIKE_BIN SPIKE_ISA OBJCOPY_BIN OBJDUMP_BIN LD_BIN PC_STAGNATION_LIMIT MAX_PROGRAM_WORDS STOP_ON_SPIKE_DONE APPEND_EXIT_STUB"

# Always set mutator config (default or user-specified)
export MUTATOR_CONFIG="${MUTATOR_CONFIG:-$PROJECT_ROOT/afl/isa_mutator/config/mutator.default.yaml}"

# Set SCHEMA_DIR for ISA mutator
export SCHEMA_DIR="${SCHEMA_DIR:-$PROJECT_ROOT/schemas}"

# Propagate crash directory to harness using the name it expects
export CRASH_LOG_DIR="$CRASH_DIR"
export SPIKE_LOG_FILE

# Golden/trace/backend defaults (from fuzzer.env or CLI overrides)
export GOLDEN_MODE="${GOLDEN_MODE:-live}"
export TRACE_MODE="${TRACE_MODE:-on}"
export EXEC_BACKEND="${EXEC_BACKEND:-verilator}"

# Tooling paths (defaults from fuzzer.env)
export SPIKE_BIN="${SPIKE_BIN:-/opt/riscv/bin/spike}"
export SPIKE_ISA="${SPIKE_ISA:-rv32im}"
export OBJCOPY_BIN="${OBJCOPY_BIN:-/opt/riscv/bin/riscv32-unknown-elf-objcopy}"
export OBJDUMP_BIN="${OBJDUMP_BIN:-/opt/riscv/bin/riscv32-unknown-elf-objdump}"
export PC_STAGNATION_LIMIT="${PC_STAGNATION_LIMIT:-512}"
export MAX_PROGRAM_WORDS="${MAX_PROGRAM_WORDS:-256}"
export STOP_ON_SPIKE_DONE="${STOP_ON_SPIKE_DONE:-1}"
export APPEND_EXIT_STUB="${APPEND_EXIT_STUB:-1}"

# Check if golden mode is enabled and enforce tool requirements
if [[ "$GOLDEN_MODE" != "off" ]]; then
  # SPIKE_BIN is required for golden mode
  if ! command -v "$SPIKE_BIN" >/dev/null 2>&1; then
    log "[!] ERROR: SPIKE_BIN not found at '$SPIKE_BIN' and not on PATH." >&2
    log "    Golden mode is enabled but Spike is not available." >&2
    log "    Install Spike or set GOLDEN_MODE=off to continue." >&2
    exit 1
  fi
  
  # Auto-adjust timeout for golden mode if using default timeout
  # Golden mode requires much longer timeouts due to Spike overhead:
  # - objcopy + ld invocation (~50-200ms per test case)
  # - Spike process spawn (~100-500ms per test case)
  # - Spike execution (2-10x slower than Verilator)
  # - Differential comparison overhead
  # Recommended: 120-180 seconds for golden mode with live differential testing
  if [[ "$TIMEOUT" == "100000" ]]; then
    log "[INFO] Golden mode enabled: auto-increasing timeout from 100s to 180s"
    log "[INFO] Override with --timeout if needed (e.g., --timeout 200000)"
    TIMEOUT="180000"
  fi
  
  # Also increase fork server initialization timeout for golden mode
  # Spike initialization adds significant overhead to the first execution
  if [[ "${AFL_FORKSRV_INIT_TMOUT:-10000}" == "10000" ]]; then
    log "[INFO] Golden mode: increasing fork server init timeout to 30s"
    export AFL_FORKSRV_INIT_TMOUT=30000
  fi
  
  # OBJCOPY_BIN is required for converting .bin to .elf
  if ! command -v "$OBJCOPY_BIN" >/dev/null 2>&1; then
    # Try 64-bit fallback
    if command -v riscv64-unknown-elf-objcopy >/dev/null 2>&1; then
      log "[INFO] Using riscv64-unknown-elf-objcopy as fallback"
      OBJCOPY_BIN="riscv64-unknown-elf-objcopy"
    else
      log "[!] ERROR: OBJCOPY_BIN '$OBJCOPY_BIN' not found and no 64-bit fallback available." >&2
      log "    Golden mode requires objcopy to convert .bin files to .elf for Spike." >&2
      exit 1
    fi
  fi

  if ! command -v "$LD_BIN" >/dev/null 2>&1; then
    if command -v riscv32-unknown-elf-ld >/dev/null 2>&1; then
      LD_BIN="riscv32-unknown-elf-ld"
    elif command -v riscv64-unknown-elf-ld >/dev/null 2>&1; then
      log "[INFO] Using riscv64-unknown-elf-ld as fallback"
      LD_BIN="riscv64-unknown-elf-ld"
    else
      log "[!] ERROR: LD_BIN '$LD_BIN' not found and no fallback available." >&2
      log "    Golden mode requires a RISC-V linker to wrap Spike inputs." >&2
      exit 1
    fi
  fi
else
  # Golden mode is off, tools are optional
  if ! command -v "$SPIKE_BIN" >/dev/null 2>&1; then
    log "[INFO] SPIKE_BIN not found (golden mode is off, this is OK)"
    unset SPIKE_BIN
  fi
  if ! command -v "$OBJCOPY_BIN" >/dev/null 2>&1; then
    log "[INFO] OBJCOPY_BIN not found (golden mode is off, this is OK)"
    unset OBJCOPY_BIN
  fi
  if ! command -v "$LD_BIN" >/dev/null 2>&1; then
    log "[INFO] LD_BIN not found (golden mode is off, this is OK)"
    unset LD_BIN
  fi
fi

# Export detected/provided tooling
if [[ -n "${SPIKE_BIN:-}" ]];   then export SPIKE_BIN; fi
if [[ -n "${SPIKE_ISA:-}" ]];   then export SPIKE_ISA; fi
if [[ -n "${OBJCOPY_BIN:-}" ]]; then export OBJCOPY_BIN; fi
if [[ -n "${OBJDUMP_BIN:-}" ]]; then export OBJDUMP_BIN; fi
if [[ -n "${LD_BIN:-}" ]];      then :; fi
export PC_STAGNATION_LIMIT MAX_PROGRAM_WORDS STOP_ON_SPIKE_DONE APPEND_EXIT_STUB

# Preserve these env vars in the target (space-separated list for AFL++)
export AFL_KEEP_ENV="CRASH_LOG_DIR MAX_CYCLES DEBUG GOLDEN_MODE EXEC_BACKEND TRACE_MODE SPIKE_BIN SPIKE_ISA OBJCOPY_BIN OBJDUMP_BIN SPIKE_LOG_FILE TOHOST_ADDR PC_STAGNATION_LIMIT MAX_PROGRAM_WORDS STOP_ON_SPIKE_DONE APPEND_EXIT_STUB RAM_BASE RAM_SIZE PROGADDR_RESET PROGADDR_IRQ STACK_ADDR STACKADDR MUTATOR_CONFIG SCHEMA_DIR"

# Optional AFL debug output (very verbose - separate from mutator/harness DEBUG)
if [[ "$AFL_DEBUG" == "1" ]]; then
  export AFL_DEBUG=1
  export AFL_DEBUG_CHILD=1
  log "[DEBUG] AFL++ debug output enabled (AFL_DEBUG=1)"
fi

# Configure AFL++ UI mode
case "$UI_MODE" in
  off)
    export AFL_NO_UI=1
    log "[UI] AFL++ status screen disabled (AFL_NO_UI=1)"
    ;;
  on)
    export AFL_FORCE_UI=1
    log "[UI] AFL++ status screen forced on (AFL_FORCE_UI=1)"
    ;;
  auto)
    # Default behavior - AFL++ auto-detects TTY
    log "[UI] AFL++ UI mode: auto (default TTY detection)"
    ;;
esac

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
  Mutator Cfg  : $MUTATOR_CONFIG
  Harness Bin  : $FUZZ_BIN
  Debug        : Mutator/Harness=$DEBUG | AFL++=$AFL_DEBUG
  UI Mode      : $UI_MODE
  Cores        : $CORES
  Timeout      : $TIMEOUT
  Mem Limit    : $MEM_LIMIT
  Seeds Dir    : $SEEDS_DIR
  Output Dir   : $CORPORA_DIR
  Work Dir     : $RUN_DIR
  Run Log      : $LOG_FILE
  Spike Log    : $SPIKE_LOG_FILE
  Runtime Log  : ${LOGS_DIR}/runtime.log
  Max Cycles   : $MAX_CYCLES
  Coverage     : $([ "$ENABLE_COVERAGE" -eq 1 ] && echo "ENABLED (Verilator structural)" || echo "DISABLED")
  RAM Base     : $RAM_BASE
  RAM Size     : $RAM_SIZE ($(printf "%d" $((RAM_SIZE_ALIGNED))) bytes)
  Reset PC     : $PROGADDR_RESET
  IRQ Vector   : $PROGADDR_IRQ
  Stack Addr   : $STACK_ADDR
  Extra AFL    : ${AFL_EXTRA_ARGS:-<none>}
  Golden Mode  : $GOLDEN_MODE
  Exec Backend : $EXEC_BACKEND
  Trace Mode   : $TRACE_MODE
  Crash Logs   : $CRASH_DIR
  tohost Addr  : ${TOHOST_ADDR:-<unset>}
  Max Program  : ${MAX_PROGRAM_WORDS} words
  PC Stall Lim : ${PC_STAGNATION_LIMIT} commits
  Stop If Spike: ${STOP_ON_SPIKE_DONE}
  SPIKE_BIN    : ${SPIKE_BIN:-<default>}
  SPIKE_ISA    : ${SPIKE_ISA:-<default>}
  OBJCOPY_BIN  : ${OBJCOPY_BIN:-<default>}
  OBJDUMP_BIN  : ${OBJDUMP_BIN:-<default>}
  LD_BIN       : ${LD_BIN:-<default>}
  AFL++ Debug  : ${AFL_DEBUG:-0}
  Backup Made  : $([ "$BACKUP_WORKDIR" -eq 1 ] && echo "Yes" || echo "No")
==========================================================
BANNER

# ---------- Preconditions ----------
if [[ "$NO_BUILD" -eq 1 ]]; then
  if [[ ! -x "$FUZZ_BIN" ]]; then
    log "[!] ERROR: Harness binary not found or not executable: $FUZZ_BIN" >&2
    log "    Run without --no-build to build it, or check the path." >&2
    exit 1
  fi
  if [[ ! -f "$MUTATOR_SO" ]]; then
    log "[!] ERROR: Mutator library not found: $MUTATOR_SO" >&2
    log "    Run without --no-build to build it, or check the path." >&2
    exit 1
  fi
fi

# Check if MUTATOR_CONFIG file exists
if [[ ! -f "$MUTATOR_CONFIG" ]]; then
  log "[!] ERROR: Mutator config file not found: $MUTATOR_CONFIG" >&2
  exit 1
fi

# Check if SCHEMA_DIR exists
if [[ ! -d "$SCHEMA_DIR" ]]; then
  log "[!] ERROR: Schema directory not found: $SCHEMA_DIR" >&2
  log "    ISA mutator requires schema files to operate." >&2
  exit 1
fi

# Check if seeds directory has at least one file
if [[ ! -d "$SEEDS_DIR" ]] || [[ -z "$(ls -A "$SEEDS_DIR" 2>/dev/null)" ]]; then
  log "[!] ERROR: Seeds directory is empty or doesn't exist: $SEEDS_DIR" >&2
  log "    AFL++ requires at least one seed file to start fuzzing." >&2
  exit 1
fi

# ---------- Build (unless --no-build) ----------
if [[ "$NO_BUILD" -eq 0 ]]; then
  log "[BUILD] Building mutator + harness..."
  log "[BUILD] Verilator coverage: $([ "$ENABLE_COVERAGE" -eq 1 ] && echo "ENABLED" || echo "DISABLED")"
  make -C "$AFL_DIR" build ENABLE_COVERAGE="$ENABLE_COVERAGE"
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
