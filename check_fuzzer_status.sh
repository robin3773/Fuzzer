#!/bin/bash
# Monitor AFL++ fuzzing progress

WORKDIR="/home/robin/HAVEN/Fuzz/workdir/07-Nov__01-42/corpora/default"

if [ ! -f "$WORKDIR/fuzzer_stats" ]; then
    echo "Fuzzer not running or stats file not found"
    exit 1
fi

echo "=== AFL++ Fuzzing Status ==="
echo ""

# Parse and display key stats
grep -E "(last_update|run_time|execs_done|execs_per_sec|corpus_count|corpus_found|bitmap_cvg|saved_crashes|saved_hangs)" "$WORKDIR/fuzzer_stats" | \
    awk -F': ' '{printf "%-20s: %s\n", $1, $2}'

echo ""
echo "=== Queue Status ==="
ls -1 "$WORKDIR/queue" | wc -l | awk '{print "Total queue entries : " $1}'

echo ""
echo "=== Recent Queue Entries ==="
ls -lth "$WORKDIR/queue" | head -6

echo ""
echo "=== Crashes and Hangs ==="
echo "Crashes: $(ls -1 $WORKDIR/crashes 2>/dev/null | wc -l)"
echo "Hangs:   $(ls -1 $WORKDIR/hangs 2>/dev/null | wc -l)"

echo ""
echo "=== Current Input Size ==="
if [ -f "$WORKDIR/.cur_input" ]; then
    wc -c "$WORKDIR/.cur_input" | awk '{print $1 " bytes"}'
fi

echo ""
echo "=== Process Status ==="
ps aux | grep -E "(afl-fuzz|afl_picorv32)" | grep -v grep | head -3
