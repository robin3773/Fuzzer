#!/bin/bash
# ============================================================
# setup_hwfuzz.sh
# Project: Verilator-based Hardware Fuzzer Framework
# Author: Mahmadul Hassan Robin
# Description:
#   Creates a clean directory structure for CPU fuzzing using
#   Verilator + AFL++. It includes folders for RTL, harness,
#   seeds, corpora, tools, docs, and traces.
# ===========================================================

set -e

# Root directory name
ROOT_DIR="hw-fuzz"

echo "🚀 Creating hardware fuzzing project structure under: $ROOT_DIR"

# --- Base directories ---
mkdir -p $ROOT_DIR/{rtl/{cpu,tb},harness,tools,afl,docs,seeds,corpora/{queue,crashes,hangs},traces,experiments,ci,verilator}

# --- Create placeholder files ---
touch $ROOT_DIR/README.md
touch $ROOT_DIR/.gitignore
touch $ROOT_DIR/LICENSE

# --- RTL folder ---
cat <<EOF > $ROOT_DIR/rtl/README.md
# RTL Directory

Place your RTL source files here.
- \`cpu/\`: contains the core design (e.g., picorv32.v, vexriscv.v)
- \`tb/\`: contains any simple testbench modules (like memory model)
- \`top.sv\`: top-level wrapper used for simulation and fuzzing.

Recommended top module signals:
- clk, rst_n
- imem[] array or AXI interface
- pc (program counter)
- trap or exception flag
EOF

# --- Harness folder ---
cat <<EOF > $ROOT_DIR/harness/README.md
# Harness Directory

Contains Verilator C++ fuzzing harness and Makefile.

- \`fuzz_picorv32.cpp\`: main harness entry (loads @@ input, runs CPU)
- \`harness_common.h\`: shared utils and macros
- \`Makefile\`: builds Verilator binary (\`Vtop\`)
EOF

# --- Tools folder ---
cat <<'EOF' > $ROOT_DIR/tools/build.sh
#!/bin/bash
# build.sh - build verilator simulation
set -e
TOP=${1:-top}
echo "🔧 Building Verilator model for $TOP..."
verilator --cc rtl/$TOP.sv --exe harness/fuzz_picorv32.cpp --top-module $TOP -O2 -Wno-fatal
make -C obj_dir -f V${TOP}.mk V${TOP}
echo "✅ Build complete: obj_dir/V${TOP}"
EOF
chmod +x $ROOT_DIR/tools/build.sh

cat <<'EOF' > $ROOT_DIR/tools/run_fuzz.sh
#!/bin/bash
# run_fuzz.sh - Run AFL++ fuzzing
set -e
SIM_BIN=${1:-./obj_dir/Vtop}
SEEDS_DIR=${2:-seeds}
OUT_DIR=${3:-corpora}
echo "🚦 Starting AFL fuzzing..."
afl-fuzz -i $SEEDS_DIR -o $OUT_DIR -t 1000 -m 2000 -- $SIM_BIN @@
EOF
chmod +x $ROOT_DIR/tools/run_fuzz.sh

cat <<'EOF' > $ROOT_DIR/tools/run_once.sh
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
EOF
chmod +x $ROOT_DIR/tools/run_once.sh

# --- AFL folder ---
cat <<EOF > $ROOT_DIR/afl/README.md
# AFL Directory

Contains AFL++ configuration and optional mutator sources.
- \`afl.conf\`: custom AFL parameters.
- \`afl_mutator/\`: for grammar or instruction-aware mutation.
EOF

# --- Seeds folder ---
cat <<EOF > $ROOT_DIR/seeds/README.md
# Seeds Directory

Initial corpus for AFL++.
Add small valid programs, e.g.:
- seed_empty.bin (NOPs or zeros)
- seed_nop_loop.bin
EOF
echo -n -e "\x13\x00\x00\x00" > $ROOT_DIR/seeds/seed_empty.bin

# --- Docs folder ---
cat <<EOF > $ROOT_DIR/docs/README.md
# Documentation

- \`design.md\`: signal map, interface description.
- \`fuzzing_recipe.md\`: mapping bytes to instructions, coverage feedback.
- \`troubleshooting.md\`: tips for debugging simulation and fuzz runs.
EOF

# --- .gitignore ---
cat <<EOF > $ROOT_DIR/.gitignore
# Ignore Verilator build artifacts
/verilator/
/obj_dir/
/corpora/
/traces/*.vcd
*.o
*.exe
*.log
*.vcd
EOF

# --- Root README ---
cat <<EOF > $ROOT_DIR/README.md
# 🧠 Hardware Fuzzing Framework (Verilator + AFL++)

This project provides a modular template for building and fuzzing hardware CPU cores (like PicoRV32 or VexRiscv).

## Directory Overview
\`\`\`
rtl/        → CPU RTL and testbench
harness/    → C++ fuzzing harness & Makefile
tools/      → build / run / triage scripts
afl/        → AFL++ config and mutators
seeds/      → initial test corpus
corpora/    → fuzzer outputs (queue, crashes)
traces/     → waveforms and logs
docs/       → design notes
\`\`\`

## Quickstart
\`\`\`bash
chmod +x tools/build.sh
./tools/build.sh top      # build verilator binary
./tools/run_fuzz.sh       # start fuzzing
\`\`\`

## Next Steps
- Place your CPU RTL under \`rtl/cpu/\`
- Add \`rtl/top.sv\` wrapper exposing clk, rst_n, pc, trap, imem[]
- Write harness/fuzz_picorv32.cpp
EOF

# --- Summary ---
echo "✅ Hardware fuzzing folder structure created successfully!"
tree $ROOT_DIR || ls -R $ROOT_DIR
