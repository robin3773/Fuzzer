# Fuzzing Harness Session Notes — 2025-10-24

This document summarizes the key changes and decisions from the recent debugging and enhancement session for the RISC-V fuzzing harness.

## Highlights
- Fixed crash logging directory creation to ensure paths are created recursively (mkdir -p behavior).
- Standardized crash logs under run-scoped directory: `workdir/<stamp>/logs/crash`.
- Ensured AFL output (corpora) lands in `workdir/<stamp>/corpora`.
- Added initial architectural crash checks (x0 write, PC alignment, memory alignment) using RVFI.
- Refactored checks into separate helper functions for clarity and future toggling.
- Added optional per-commit DUT trace writer at `workdir/<stamp>/traces/dut.trace`.
- Anchored relative crash directory paths to project root (derived from the harness binary path) to avoid `afl/workdir/...` drift.
- Cleaned up build system so top-level `workdir/corpora` and `workdir/traces` are not created by `make`.
- Tightened `.gitignore` to exclude any `corpora/`, `traces/`, and everything under `workdir/` anywhere in the tree.

## Files changed
- `afl_harness/utils.hpp`
  - `ensure_dir()` now creates parent directories recursively and prints errors if creation fails.
- `afl_harness/harness_config.hpp`
  - Includes `<cstdio>` for `printf`.
  - Resolves relative `CRASH_LOG_DIR` against project root using `/proc/self/exe` to anchor logs under repo root.
- `afl_harness/cpu_iface.hpp`
  - Added RVFI accessors: `rvfi_valid`, `rvfi_pc_wdata`, `rvfi_rd_addr`, `rvfi_rd_wdata`, `rvfi_mem_addr`, `rvfi_mem_rmask`, `rvfi_mem_wmask`.
- `afl_harness/cpu_picorv32.cpp`
  - Implemented the new RVFI accessors using Verilated `rvfi_*` signals.
- `afl_harness/harness_main.cpp`
  - Installed signal handlers and integrated CrashLogger.
  - Added per-commit DUT trace writing when `TRACE_DIR` is set.
  - Added retire-time checks (x0 write, PC misalignment, load/store misalignment) via dedicated helper functions.
- `afl_harness/trace.hpp`
  - New: `TraceWriter` and `CommitRec` for CSV-like DUT trace output.
- `afl/Makefile`
  - `dirs` no longer creates top-level `workdir/corpora` and `workdir/traces`; only ensures `seeds`.
- `run.sh`
  - Exports `CRASH_LOG_DIR` as an absolute run-scoped path and preserves it with `AFL_KEEP_ENV` (space-separated).
  - Fixed `--debug` to actually enable `DEBUG_MUTATOR` and AFL debug.
  - Creates `TRACES_DIR` under the stamped run dir and preserves it via `AFL_KEEP_ENV`.
  - Uses run-scoped `corpora` for `-o` to AFL.
- `.gitignore`
  - Ignore rules simplified and strengthened: ignore any `corpora/`, any `traces/`, and all of `workdir/`.

## Behavior changes
- Crashes are consistently written to `workdir/<stamp>/logs/crash` (or `CRASH_LOG_DIR` if set absolute).
- AFL outputs are under `workdir/<stamp>/corpora`.
- Optional DUT commit trace at `workdir/<stamp>/traces/dut.trace` when `TRACE_DIR` is set (set by `run.sh`).
- Early crash detection for:
  - Non-zero writes to x0.
  - PC misalignment (at least 2-byte alignment enforced).
  - Misaligned or irregular load/store masks.

## How to run (quick)
- Scripted session with per-run directories under `workdir/<stamp>`:
  - `./run.sh` (or pass options like `--cores`, `--timeout`, `--debug`).
- Single input directly:
  - `CRASH_LOG_DIR=workdir/logs/crash ./afl/afl_picorv32 ./seeds/firmware.bin`

## Known artifacts/validation
- Build checks performed; harness compiles with Verilator model and AFL toolchain.
- Created and validated run-scoped directories for logs, corpora, traces.

## Next steps (planned)
- Golden model integration (Spike):
  - Option A: external Spike process with `-l` log parsing and live diff vs DUT per commit.
  - Option B: embed Spike as a library; lock-step compare without log parsing.
- Add more checks:
  - `write_on_trap` (no commit effects when trap asserted),
  - no-retire watchdog (detect livelock earlier than global timeout),
  - PC-stuck detection.
- Feature flags for checks (env-driven toggles) and more detailed logging (fsync, reason details).

## Notes
- Environment variables preserved to the target (AFL): `CRASH_LOG_DIR`, `TRACE_DIR`, `MAX_CYCLES`, `RV32_STRATEGY`, `RV32_ENABLE_C`.
- You can override objdump/XLEN via env and add them to `AFL_KEEP_ENV` if needed.

