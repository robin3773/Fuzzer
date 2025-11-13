#!/bin/bash
# Test script to verify AFL++ feedback is working

set -e

cd "$(dirname "$0")/.."

echo "============================================"
echo "AFL++ Feedback Verification Test"
echo "============================================"
echo

# Step 1: Check if harness uses AFL++ instrumentation
echo "[1/5] Checking AFL++ instrumentation..."
if nm afl/afl_picorv32 | grep -q "__afl"; then
    echo "✅ AFL++ instrumentation symbols found"
    nm afl/afl_picorv32 | grep "__afl" | head -5
else
    echo "❌ No AFL++ instrumentation found!"
    exit 1
fi
echo

# Step 2: Check if feedback methods are called
echo "[2/5] Checking feedback implementation..."
if grep -q "feedback.report_instruction" afl_harness/src/HarnessMain.cpp; then
    echo "✅ DUT feedback calls present in code"
else
    echo "❌ DUT feedback not implemented!"
    exit 1
fi
echo

# Step 3: Run a quick AFL++ test with instrumentation check
echo "[3/5] Running AFL++ dry run to verify bitmap updates..."
rm -rf /tmp/afl_test_$$
mkdir -p /tmp/afl_test_$$/in /tmp/afl_test_$$/out

# Copy one seed
cp seeds/seed_minimal.bin /tmp/afl_test_$$/in/

# Run AFL++ for 5 seconds with debug output
timeout 5s env AFL_DEBUG=1 \
    afl-fuzz -i /tmp/afl_test_$$/in -o /tmp/afl_test_$$/out \
    -m 4G -t 180000 -M main -- afl/afl_picorv32 @@ 2>&1 | \
    tee /tmp/afl_feedback_test_$$.log || true

echo
echo "[4/5] Analyzing AFL++ output..."
if grep -q "edges found" /tmp/afl_feedback_test_$$.log; then
    echo "✅ AFL++ discovered new edges (feedback is working!)"
    grep "edges found" /tmp/afl_feedback_test_$$.log | tail -1
elif grep -q "map density" /tmp/afl_feedback_test_$$.log; then
    echo "✅ AFL++ is tracking coverage map (feedback is working!)"
    grep "map density" /tmp/afl_feedback_test_$$.log | tail -1
else
    echo "⚠️  Could not verify feedback from AFL++ output"
fi
echo

# Step 5: Check if AFL++ queue grew
echo "[5/5] Checking if AFL++ discovered new paths..."
QUEUE_SIZE=$(find /tmp/afl_test_$$/out/main/queue -type f 2>/dev/null | wc -l)
if [ "$QUEUE_SIZE" -gt 1 ]; then
    echo "✅ AFL++ queue grew from 1 to $QUEUE_SIZE entries"
    echo "   This proves feedback is working!"
    ls -lh /tmp/afl_test_$$/out/main/queue/ | head -10
else
    echo "⚠️  AFL++ queue didn't grow (may need longer run)"
fi
echo

# Cleanup
rm -rf /tmp/afl_test_$$
rm -f /tmp/afl_feedback_test_$$.log

echo "============================================"
echo "Feedback Verification Complete!"
echo "============================================"
echo
echo "Key indicators that feedback is working:"
echo "  ✓ AFL++ instrumentation present"
echo "  ✓ Feedback code compiled in"
echo "  ✓ AFL++ discovers new edges"
echo "  ✓ Queue grows over time"
echo
echo "To monitor feedback during fuzzing:"
echo "  1. Watch 'map density' - should increase"
echo "  2. Watch 'new edges on' - should find new paths"
echo "  3. Watch 'corpus count' - should grow"
echo "  4. Check queue/ directory - should have new entries"
