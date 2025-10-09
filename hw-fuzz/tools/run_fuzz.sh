#!/bin/bash
# run_fuzz.sh - Run AFL++ fuzzing
set -e
SIM_BIN=${1:-./obj_dir/Vtop}
SEEDS_DIR=${2:-seeds}
OUT_DIR=${3:-corpora}
echo "🚦 Starting AFL fuzzing..."
afl-fuzz -i $SEEDS_DIR -o $OUT_DIR -t 1000 -m 2000 -- $SIM_BIN @@
