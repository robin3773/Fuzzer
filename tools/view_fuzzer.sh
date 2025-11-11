#!/bin/bash
# Real-time AFL++ fuzzer status viewer

# Find the most recent run directory
LATEST_RUN=$(ls -td /home/robin/HAVEN/Fuzz/workdir/*/ 2>/dev/null | head -1)

if [ -z "$LATEST_RUN" ]; then
    echo "No fuzzing run found in workdir"
    exit 1
fi

STATS_FILE="${LATEST_RUN}corpora/default/fuzzer_stats"

if [ ! -f "$STATS_FILE" ]; then
    echo "Fuzzer stats file not found: $STATS_FILE"
    echo "Run might not have started yet."
    exit 1
fi

echo "=== AFL++ Fuzzer Status ===" 
echo "Run Directory: $LATEST_RUN"
echo ""

# Continuous display
watch -n 2 "cat <<'EOF'
╔══════════════════════════════════════════════════════════════════╗
║                    AFL++ FUZZER STATUS                           ║
╚══════════════════════════════════════════════════════════════════╝

$(grep -E '(run_time|execs_done|execs_per_sec|corpus_count|corpus_found|bitmap_cvg|saved_crashes|saved_hangs|last_find)' $STATS_FILE | \
  awk -F': ' '{printf \"%-20s: %s\n\", \$1, \$2}')

╔══════════════════════════════════════════════════════════════════╗
║                    QUEUE & COVERAGE                              ║
╚══════════════════════════════════════════════════════════════════╝

Queue entries: $(ls -1 ${LATEST_RUN}corpora/default/queue 2>/dev/null | wc -l)
Crashes:       $(ls -1 ${LATEST_RUN}corpora/default/crashes 2>/dev/null | grep -v README | wc -l)
Hangs:         $(ls -1 ${LATEST_RUN}corpora/default/hangs 2>/dev/null | grep -v README | wc -l)

╔══════════════════════════════════════════════════════════════════╗
║                    RECENT ACTIVITY                               ║
╚══════════════════════════════════════════════════════════════════╝

$(ls -lth ${LATEST_RUN}corpora/default/queue 2>/dev/null | head -6 | tail -5 | \
  awk '{printf \"%-10s %s\n\", \$5, \$9}')

╔══════════════════════════════════════════════════════════════════╗
║                    LOG FILES                                     ║
╚══════════════════════════════════════════════════════════════════╝

Harness+Mutator: ${LATEST_RUN}logs/harness.log ($(du -h ${LATEST_RUN}logs/harness.log 2>/dev/null | cut -f1))
Spike Log:       ${LATEST_RUN}spike.log ($(du -h ${LATEST_RUN}spike.log 2>/dev/null | cut -f1))
Fuzz Log:        ${LATEST_RUN}fuzz.log ($(du -h ${LATEST_RUN}fuzz.log 2>/dev/null | cut -f1))

EOF
"
