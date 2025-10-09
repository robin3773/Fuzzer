#!/bin/bash
# build.sh - build verilator simulation
set -e
TOP=${1:-top}
echo "🔧 Building Verilator model for $TOP..."
verilator --cc rtl/$TOP.sv --exe harness/fuzz_picorv32.cpp --top-module $TOP -O2 -Wno-fatal
make -C obj_dir -f V${TOP}.mk V${TOP}
echo "✅ Build complete: obj_dir/V${TOP}"
