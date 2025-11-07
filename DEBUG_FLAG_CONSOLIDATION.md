# Debug Flag Consolidation

## Overview
Consolidated all debug and logging controls into a single `DEBUG_MUTATOR` environment variable. This simplifies configuration and provides a consistent debug experience across the entire fuzzing infrastructure.

## What Changed

### Before (Multiple Flags)
Debug and logging were controlled by multiple environment variables:
```bash
export DEBUG_MUTATOR="1"     # Enable mutator debug
export DEBUG_LOG="1"         # Enable debug file logging
export FUZZER_QUIET="1"      # Silence output
```

### After (Single Flag)
One master debug switch controls everything:
```bash
export DEBUG_MUTATOR="1"     # Enables ALL debug features
# When DEBUG_MUTATOR=1:
#   - Verbose logging to stdout (via harness_log)
#   - Function entry/exit tracing
#   - Debug file output to afl/isa_mutator/logs/mutator_debug.log
#   - Illegal instruction mutation logging

export DEBUG_MUTATOR="0"     # Normal operation (default)
# When DEBUG_MUTATOR=0:
#   - Minimal output
#   - No function tracing
#   - No debug file creation
```

## Code Changes

### 1. MutatorDebug.hpp
**File**: `afl/isa_mutator/include/fuzz/mutator/MutatorDebug.hpp`

Removed separate `DEBUG_LOG` environment variable:
```cpp
// Before: Two separate flags
const char *dbg = std::getenv("DEBUG_MUTATOR");
S.enabled = (dbg && std::strcmp(dbg, "0") != 0);
const char *lg = std::getenv("DEBUG_LOG");
if (lg && std::strcmp(lg, "0") != 0) {
  S.log_to_file = true;
  S.path = (std::strcmp(lg, "1") == 0)
         ? "afl/isa_mutator/logs/mutator_debug.log"
         : lg;
  S.fp = std::fopen(S.path.c_str(), "ab");
}

// After: Single flag controls everything
const char *dbg = std::getenv("DEBUG_MUTATOR");
S.enabled = (dbg && std::strcmp(dbg, "0") != 0);

// If debug is enabled, also enable file logging
if (S.enabled) {
  S.log_to_file = true;
  S.path = "afl/isa_mutator/logs/mutator_debug.log";
  S.fp = std::fopen(S.path.c_str(), "ab");
}
```

### 2. fuzzer.env
**File**: `fuzzer.env`

Consolidated debug configuration:
```bash
# Before:
export DEBUG_MUTATOR="0"
export FUZZER_QUIET="0"

# After:
# ---------- Debug Configuration ----------
# Single debug flag controls ALL logging and output:
# - When DEBUG_MUTATOR=1: Enables verbose logging, function tracing, and debug file output
# - When DEBUG_MUTATOR=0: Normal operation, minimal output
export DEBUG_MUTATOR="0"                # Master debug switch (0=off, 1=on)
```

Removed obsolete `FUZZER_QUIET`:
```bash
# Before:
export FUZZER_QUIET="0"

# After: (removed, with note)
# Note: FUZZER_QUIET is deprecated. Use DEBUG_MUTATOR=0 for normal operation,
# or DEBUG_MUTATOR=1 for verbose debug output. The harness automatically redirects
# all output to logs/harness.log to keep AFL++ stdio clean.
```

### 3. run.sh
**File**: `run.sh`

Removed `QUIET_MODE` and `--quiet` flag:
```bash
# Removed:
QUIET_MODE="${FUZZER_QUIET:-0}"
export FUZZER_QUIET="${FUZZER_QUIET:-$QUIET_MODE}"
if [[ "$QUIET_MODE" == "1" ]]; then
  export GOLDEN_MODE="${GOLDEN_MODE:-off}"
  export TRACE_MODE="${TRACE_MODE:-off}"
fi
--quiet) QUIET_MODE=1; shift ;;

# Kept:
export DEBUG_MUTATOR="${DEBUG_MUTATOR:-0}"
--debug) DEBUG_MUTATOR=1; AFL_DEBUG_FLAG=1; shift ;;
```

### 4. Documentation Updates

**CONFIG.md**:
```markdown
# Before:
| `DEBUG_MUTATOR` | `0` | Enable mutator debug logging |
| `FUZZER_QUIET` | `0` | Silence harness/mutator output |

# After:
| `DEBUG_MUTATOR` | `0` | Master debug switch: enables all logging, function tracing, and debug file output |

**Note**: The harness automatically redirects all stdout/stderr to `logs/harness.log` 
to keep AFL++ stdio clean. Use `DEBUG_MUTATOR=1` for verbose debug output.
```

**afl/isa_mutator/README.md**:
```markdown
### Environment Variables
- `DEBUG_MUTATOR` - Master debug switch: enables all logging, function tracing, and debug file output (0/1)

**Note**: When `DEBUG_MUTATOR=1`, the mutator automatically:
- Enables verbose logging to stdout (via harness_log)
- Enables function entry/exit tracing
- Creates debug log file at `afl/isa_mutator/logs/mutator_debug.log`
- Logs illegal instruction mutations for analysis
```

## Debug Features Enabled by DEBUG_MUTATOR=1

### 1. Verbose Logging
All mutator operations logged to `logs/harness.log`:
```
[INFO] Config file opened successfully: afl/isa_mutator/config/mutator.default.yaml
[INFO] Loaded config: afl/isa_mutator/config/mutator.default.yaml
[INFO] strategy=IR verbose=true enable_c=false decode_prob=60 ...
```

### 2. Function Tracing
Entry/exit logging for all major functions:
```
[Fn Start  ]  ISAMutator.cpp::afl_custom_fuzz
[Fn Start  ]  ISAMutator.cpp::mutate_with_strategy
[Fn End    ]  ISAMutator.cpp::mutate_with_strategy
[Fn End    ]  ISAMutator.cpp::afl_custom_fuzz
```

### 3. Debug File Output
Detailed logs written to `afl/isa_mutator/logs/mutator_debug.log`:
```
[ILLEGAL] replace_instruction()
  before = 0x00000013
  after  = 0x00000093
```

### 4. Illegal Instruction Logging
Tracks mutations that produce illegal encodings:
```
[ILLEGAL] mutate_immediate()
  before = 0x00a00093
  after  = 0xfff00093
```

## Usage Examples

### Normal Fuzzing (Production)
```bash
export DEBUG_MUTATOR="0"
./run.sh --cores 8
```
- Minimal output
- All logs go to `logs/harness.log`
- AFL++ stdio remains clean

### Debug Fuzzing (Development)
```bash
export DEBUG_MUTATOR="1"
./run.sh --cores 1 --debug
# OR
./run.sh --cores 1 --debug
```
- Verbose logging enabled
- Function tracing active
- Debug file created at `afl/isa_mutator/logs/mutator_debug.log`
- AFL_DEBUG=1 also set for AFL++ verbose mode

### Quick Debug Toggle
```bash
# Enable debug for one run
DEBUG_MUTATOR=1 ./afl/afl_picorv32 seeds/firmware.bin

# Normal run
./afl/afl_picorv32 seeds/firmware.bin
```

## Benefits

### 1. Simplicity
One flag instead of multiple (`DEBUG_MUTATOR`, `DEBUG_LOG`, `FUZZER_QUIET`).

### 2. Consistency
All debug features enabled/disabled together - no partial states.

### 3. Predictability
`DEBUG_MUTATOR=1` always means "full debug mode", `=0` always means "production mode".

### 4. Discoverability
Users only need to know one flag name instead of remembering multiple flags.

### 5. Less Configuration
Fewer environment variables to set and document.

## Migration Guide

### For Users

**Old approach**:
```bash
# Enable debug with multiple flags
export DEBUG_MUTATOR="1"
export DEBUG_LOG="afl/isa_mutator/logs/debug.log"
export FUZZER_QUIET="0"
./run.sh
```

**New approach**:
```bash
# Single flag enables everything
export DEBUG_MUTATOR="1"
./run.sh --debug
```

**Quiet mode (old)**:
```bash
export FUZZER_QUIET="1"
./run.sh
```

**Quiet mode (new)**:
```bash
# Default behavior - output already goes to log files
./run.sh
# No special flag needed
```

### For Developers

Environment variables removed:
- `DEBUG_LOG` - Now automatic when `DEBUG_MUTATOR=1`
- `FUZZER_QUIET` - Output redirection is always active

Environment variables kept:
- `DEBUG_MUTATOR` - Master debug switch (0=off, 1=on)

Debug state is accessed via:
```cpp
// Check if debug is enabled
MutatorDebug::state().enabled

// Check if debug file is open
MutatorDebug::state().fp
```

## Output Redirection

Regardless of `DEBUG_MUTATOR` setting, all output is redirected:

### Always Redirected
- **Harness output** → `logs/harness.log` (via `hwfuzz::harness_log()`)
- **Mutator output** → `logs/harness.log` (via `hwfuzz::harness_log()`)

### Debug Mode Only
- **Debug file** → `afl/isa_mutator/logs/mutator_debug.log` (when `DEBUG_MUTATOR=1`)

### AFL++ Stdio
- **stdout/stderr** - Reserved for AFL++ communication
- Never used by harness or mutator (except AFL++ debug messages when `AFL_DEBUG=1`)

## Verification

Build succeeds with changes:
```bash
make -C afl clean && make -C afl build
```

Test debug mode:
```bash
DEBUG_MUTATOR=1 ./afl/afl_picorv32 seeds/firmware.bin
# Check for debug output in logs/harness.log
# Check for debug file at afl/isa_mutator/logs/mutator_debug.log
```

Test normal mode:
```bash
DEBUG_MUTATOR=0 ./afl/afl_picorv32 seeds/firmware.bin
# Should see minimal output
# No debug file created
```

## Related Files

### Modified Files
- `afl/isa_mutator/include/fuzz/mutator/MutatorDebug.hpp` - Removed DEBUG_LOG
- `fuzzer.env` - Removed FUZZER_QUIET, documented DEBUG_MUTATOR
- `run.sh` - Removed --quiet flag and QUIET_MODE handling
- `CONFIG.md` - Updated debug flag documentation
- `afl/isa_mutator/README.md` - Documented debug features

### Unchanged Files
- `afl/isa_mutator/src/DebugUtils.cpp` - Already used DEBUG_MUTATOR
- `include/hwfuzz/QuietLog.hpp` - Still checks FUZZER_QUIET (backward compat)
- `include/hwfuzz/Log.hpp` - Output redirection logic unchanged

## Summary

Consolidated debug controls into a single `DEBUG_MUTATOR` environment variable that enables all debug features when set to `1`. This simplifies configuration, reduces user confusion, and provides consistent debug behavior across the entire fuzzing infrastructure.

**Key takeaway**: `DEBUG_MUTATOR=1` enables everything, `DEBUG_MUTATOR=0` (default) is production mode.
