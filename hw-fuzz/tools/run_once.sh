#!/bin/bash
# run_once.sh - Run simulator on a single input and save trace
set -e
SIM_BIN=${1:-./obj_dir/Vtop}
INPUT_FILE=${2:-seeds/seed_empty.bin}
OUT_NAME=$(basename $INPUT_FILE)
OUT_DIR=traces
mkdir -p $OUT_DIR
echo "▶️ Running single test input: $INPUT_FILE"
$SIM_BIN $INPUT_FILE > $OUT_DIR/${OUT_NAME%.bin}.log 2>&1 || true
echo "💾 Log saved to $OUT_DIR/${OUT_NAME%.bin}.log"
