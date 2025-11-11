# Fuzzer Output Management - Summary

## Problem
The AFL++ fuzzer was very slow (~0.25 execs/sec) and previous harness logs grew to 1.6GB, causing:
- Disk I/O bottleneck
- Cluttered stdout/stderr interfering with AFL++ UI
- Difficulty monitoring fuzzer progress

## Solution
Redirected ALL harness and ISA mutator output to a dedicated log file, leaving stdout/stderr exclusively for AFL++.

## Changes Made

### 1. Updated Logging Infrastructure
- **HarnessConfig.hpp**: Changed `std::cout` → `std::fprintf(hwfuzz::harness_log())`
- **Utils.hpp**: Changed `std::cout/cerr` → `std::fprintf(hwfuzz::harness_log())`
- Both now use the centralized logging system that respects `HARNESS_STDIO_LOG`

### 2. Updated run.sh
- Created `LOGS_DIR` directory structure
- Changed log location: `${RUN_DIR}/harness_stdio.log` → `${LOGS_DIR}/harness.log`
- Updated banner to show "Harness+Mutator Log" (combined output)
- Already exported `HARNESS_STDIO_LOG` to AFL_KEEP_ENV (no change needed)

### 3. Created Monitoring Tools
- **check_fuzzer_status.sh**: One-time status snapshot
- **view_fuzzer.sh**: Live dashboard with auto-refresh (using `watch`)

## Output Routing

```
ISA Mutator (libisa_mutator.so)
  └─> hwfuzz::harness_log()
        └─> $HARNESS_STDIO_LOG
              └─> ${RUN_DIR}/logs/harness.log

AFL Harness (afl_picorv32)  
  └─> hwfuzz::harness_log()
        └─> $HARNESS_STDIO_LOG
              └─> ${RUN_DIR}/logs/harness.log

AFL++ (afl-fuzz)
  └─> stdout/stderr (TUI + status updates)
```

## Usage

### Start Fuzzing (silent mode for harness/mutator)
```bash
./run.sh
```

All harness and mutator output now goes to:
- `workdir/<timestamp>/logs/harness.log`

### Monitor Progress
```bash
# Live dashboard (refreshes every 2 seconds)
./view_fuzzer.sh

# One-time snapshot
./check_fuzzer_status.sh
```

### Check Logs
```bash
# View harness/mutator output
tail -f workdir/$(ls -t workdir | head -1)/logs/harness.log

# View AFL++ startup log
tail -f workdir/$(ls -t workdir | head -1)/fuzz.log

# View Spike golden model errors
tail -f workdir/$(ls -t workdir | head -1)/spike.log
```

## Benefits

✅ **Clean AFL++ UI**: Only AFL++ writes to stdout/stderr  
✅ **Better Performance**: Less disk I/O from verbose logging  
✅ **Easy Debugging**: All harness/mutator output in one place  
✅ **Live Monitoring**: `view_fuzzer.sh` shows real-time stats  
✅ **No Code Changes**: Mutator already used `hwfuzz::harness_log()`  

## File Locations

| Component | Output Destination |
|-----------|-------------------|
| ISA Mutator | `${RUN_DIR}/logs/harness.log` |
| AFL Harness | `${RUN_DIR}/logs/harness.log` |
| AFL++ Startup | `${RUN_DIR}/fuzz.log` |
| AFL++ TUI | stdout/stderr (terminal) |
| Spike | `${RUN_DIR}/spike.log` |
| Crashes | `${RUN_DIR}/logs/crash/` |
| Traces | `${RUN_DIR}/traces/` |

## Next Steps

The fuzzer is now configured to:
1. Redirect all harness/mutator output → log file
2. Keep AFL++ output on stdout/stderr for UI
3. Provide live monitoring tools

Run `./view_fuzzer.sh` to see live stats! 📊
