#!/bin/bash
# Monitor Verilator coverage growth during fuzzing

COVERAGE_FILE="${1:-workdir/traces/coverage.dat}"
INTERVAL="${2:-5}"

echo "════════════════════════════════════════════════════════════"
echo "  Verilator Coverage Monitor"
echo "════════════════════════════════════════════════════════════"
echo "Coverage file: $COVERAGE_FILE"
echo "Update interval: ${INTERVAL}s"
echo "Press Ctrl+C to stop"
echo "════════════════════════════════════════════════════════════"
echo

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

prev_lines=0
prev_toggles=0
prev_fsm=0

while true; do
  if [ -f "$COVERAGE_FILE" ]; then
    # Parse coverage.dat for different coverage types
    # C <type> <count> <file> <line> ...
    # Type 0 = line coverage
    # Type 1 = toggle coverage  
    # Type 2 = FSM coverage
    
    lines=$(grep "^C 0" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    toggles=$(grep "^C 1" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    fsm=$(grep "^C 2" "$COVERAGE_FILE" 2>/dev/null | wc -l)
    
    # Calculate deltas
    delta_lines=$((lines - prev_lines))
    delta_toggles=$((toggles - prev_toggles))
    delta_fsm=$((fsm - prev_fsm))
    
    # Format timestamp
    timestamp=$(date '+%H:%M:%S')
    
    # Print status with colors
    echo -ne "${CYAN}[$timestamp]${NC} "
    echo -ne "${GREEN}Lines: $lines${NC} "
    [ $delta_lines -gt 0 ] && echo -ne "(${YELLOW}+$delta_lines${NC}) "
    echo -ne "| ${GREEN}Toggles: $toggles${NC} "
    [ $delta_toggles -gt 0 ] && echo -ne "(${YELLOW}+$delta_toggles${NC}) "
    echo -ne "| ${GREEN}FSM: $fsm${NC} "
    [ $delta_fsm -gt 0 ] && echo -ne "(${YELLOW}+$delta_fsm${NC})"
    echo
    
    prev_lines=$lines
    prev_toggles=$toggles
    prev_fsm=$fsm
    
  else
    echo -e "${RED}[$(date '+%H:%M:%S')]${NC} Waiting for $COVERAGE_FILE..."
  fi
  
  sleep "$INTERVAL"
done
