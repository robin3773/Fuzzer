#!/usr/bin/env bash
# setup_hwfuzz.sh
# Create the HW-Fuzz project folders and empty placeholder files. 
# Usage: chmod +x setup_hwfuzz.sh && ./setup_hwfuzz.sh
set -euo pipefail

ROOT=${1:-.}
WORKDIR="$ROOT/workdir"

info(){ printf "\033[1;34m[INFO]\033[0m %s\n" "$*"; }
warn(){ printf "\033[1;33m[WARN]\033[0m %s\n" "$*"; }
die(){ printf "\033[1;31m[ERROR]\033[0m %s\n" "$*"; exit 1; }

info "Creating directories under: $ROOT"

dirs=(
  "$ROOT/rtl"
  "$ROOT/rtl/cpu"
  "$ROOT/rtl/tb"
  "$ROOT/harness"
  "$ROOT/tools"
  "$ROOT/seeds"
  "$ROOT/afl"
  "$ROOT/corpora"
  "$ROOT/corpora/queue"
  "$ROOT/corpora/crashes"
  "$ROOT/corpora/hangs"
  "$ROOT/traces"
  "$WORKDIR"
  "$WORKDIR/logs"
  "$WORKDIR/logs/build"
  "$WORKDIR/logs/sim"
  "$WORKDIR/logs/afl"
  "$WORKDIR/traces"
  "$WORKDIR/corpora"
  "$ROOT/docs"
  "$ROOT/experiments"
  "$ROOT/ci"
  "$ROOT/verilator"
)

for d in "${dirs[@]}"; do
  if [ ! -d "$d" ]; then
    mkdir -p "$d"
    info "mkdir $d"
  else
    info "exists $d"
  fi
done

info "Creating empty placeholder files (will NOT overwrite existing files)"

# helper to create file only if it doesn't exist
ensure_file() {
  local f="$1"
  if [ -e "$f" ]; then
    info "exists $f"
  else
    # ensure parent dir exists
    mkdir -p "$(dirname "$f")"
    touch "$f"
    info "created $f"
  fi
}

# top-level placeholders
ensure_file "$ROOT/.gitignore"
ensure_file "$ROOT/README.md"
ensure_file "$ROOT/LICENSE"

# rtl/harness placeholders
ensure_file "$ROOT/rtl/top.sv"
ensure_file "$ROOT/rtl/cpu/picorv32.v"    # user said they have this already; we won't overwrite
ensure_file "$ROOT/rtl/tb/sim_mem.sv"

# harness
ensure_file "$ROOT/harness/fuzz_picorv32.cpp"
ensure_file "$ROOT/harness/imem_loader.cpp"
ensure_file "$ROOT/harness/Makefile"

# tools
ensure_file "$ROOT/tools/build.sh"
ensure_file "$ROOT/tools/build_and_log.sh"
ensure_file "$ROOT/tools/run_once.sh"
ensure_file "$ROOT/tools/run_once_log.sh"
ensure_file "$ROOT/tools/run_fuzz.sh"
ensure_file "$ROOT/tools/run_fuzz_log.sh"

# seeds and afl
ensure_file "$ROOT/seeds/seed_empty.bin"
ensure_file "$ROOT/afl/afl.conf"

# workdir env
ensure_file "$WORKDIR/.env_workdir"

# docs & experiments
ensure_file "$ROOT/docs/design.md"
ensure_file "$ROOT/docs/fuzzing_recipe.md"
ensure_file "$ROOT/experiments/exp_config.yaml"

# ci and verilator placeholders
ensure_file "$ROOT/ci/smoke_test.sh"
ensure_file "$ROOT/verilator/.gitkeep"

# mark script placeholders executable where appropriate
make_exec() {
  local f="$1"
  if [ -f "$f" ]; then
    chmod +x "$f" || true
    info "chmod +x $f"
  fi
}

make_exec "$ROOT/tools/build.sh"
make_exec "$ROOT/tools/build_and_log.sh"
make_exec "$ROOT/tools/run_once.sh"
make_exec "$ROOT/tools/run_once_log.sh"
make_exec "$ROOT/tools/run_fuzz.sh"
make_exec "$ROOT/tools/run_fuzz_log.sh"
make_exec "$ROOT/ci/smoke_test.sh"

# create symlink to latest workdir
ln -sfn "$WORKDIR" "$ROOT/workdir_latest"
info "symlink workdir_latest -> $WORKDIR"

info "Done. Empty files created where needed. Edit files to add content."

cat <<EOF

Next recommended steps:
  - Add your picorv32.v to rtl/cpu/ (if not already).
  - Edit rtl/top.sv and harness/fuzz_picorv32.cpp to match your core's ports.
  - Populate tools/* scripts with your preferred commands.
  - Commit the initial skeleton:
      git add .
      git commit -m "project skeleton created"
      git push -u origin main

EOF
