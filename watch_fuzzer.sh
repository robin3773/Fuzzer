#!/usr/bin/env bash
# ==========================================================
# AFL++ Fuzzer Live Status Dashboard
# ==========================================================
# Usage: ./watch_fuzzer.sh [run_dir]
#
# Shows real-time fuzzing statistics without the AFL TUI
# ==========================================================

WATCH_INTERVAL="${WATCH_INTERVAL:-3}"  # Update every 3 seconds

# Find most recent run dir if not specified
if [[ -n "$1" ]]; then
  RUN_DIR="$1"
else
  RUN_DIR=$(ls -td workdir/*/corpora/default 2>/dev/null | head -1)
  if [[ -z "$RUN_DIR" ]]; then
    echo "No active fuzzing session found. Usage: $0 [run_dir]"
    exit 1
  fi
fi

STATS_FILE="$RUN_DIR/fuzzer_stats"

if [[ ! -f "$STATS_FILE" ]]; then
  echo "Fuzzer stats not found: $STATS_FILE"
  exit 1
fi

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

clear

while true; do
  # Move cursor to top
  tput cup 0 0
  
  echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"
  echo -e "${BOLD}${CYAN}        AFL++ Hardware Fuzzing Dashboard${NC}"
  echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"
  echo ""
  
  if [[ ! -f "$STATS_FILE" ]]; then
    echo -e "${RED}[!] Fuzzer stopped or stats file missing${NC}"
    break
  fi
  
  # Parse stats
  START_TIME=$(grep "^start_time" "$STATS_FILE" | awk '{print $3}')
  LAST_UPDATE=$(grep "^last_update" "$STATS_FILE" | awk '{print $3}')
  RUN_TIME=$(grep "^run_time" "$STATS_FILE" | awk '{print $3}')
  EXECS_DONE=$(grep "^execs_done" "$STATS_FILE" | awk '{print $3}')
  EXECS_PER_SEC=$(grep "^execs_per_sec" "$STATS_FILE" | awk '{print $3}')
  CORPUS_COUNT=$(grep "^corpus_count" "$STATS_FILE" | awk '{print $3}')
  CORPUS_FOUND=$(grep "^corpus_found" "$STATS_FILE" | awk '{print $3}')
  BITMAP_CVG=$(grep "^bitmap_cvg" "$STATS_FILE" | awk '{print $3}')
  CRASHES=$(grep "^saved_crashes" "$STATS_FILE" | awk '{print $3}')
  HANGS=$(grep "^saved_hangs" "$STATS_FILE" | awk '{print $3}')
  PENDING=$(grep "^pending_total" "$STATS_FILE" | awk '{print $3}')
  CYCLES=$(grep "^cycles_done" "$STATS_FILE" | awk '{print $3}')
  
  # Calculate runtime in human format
  HOURS=$((RUN_TIME / 3600))
  MINS=$(((RUN_TIME % 3600) / 60))
  SECS=$((RUN_TIME % 60))
  
  # Display stats
  echo -e "${BOLD}⏱  Runtime:${NC}         ${GREEN}${HOURS}h ${MINS}m ${SECS}s${NC}"
  echo -e "${BOLD}🎯 Executions:${NC}      ${GREEN}${EXECS_DONE}${NC}"
  echo -e "${BOLD}⚡ Speed:${NC}           ${YELLOW}${EXECS_PER_SEC} exec/sec${NC}"
  echo ""
  echo -e "${BOLD}📚 Corpus:${NC}          ${CYAN}${CORPUS_COUNT} total${NC} (${CORPUS_FOUND} new)"
  echo -e "${BOLD}🗺  Coverage:${NC}        ${BLUE}${BITMAP_CVG}${NC}"
  echo -e "${BOLD}📋 Queue:${NC}           ${PENDING} pending"
  echo -e "${BOLD}🔄 Cycles:${NC}          ${CYCLES}"
  echo ""
  
  if [[ "$CRASHES" -gt 0 ]]; then
    echo -e "${BOLD}💥 Crashes:${NC}         ${RED}${CRASHES}${NC} 🚨"
  else
    echo -e "${BOLD}💥 Crashes:${NC}         ${GREEN}${CRASHES}${NC}"
  fi
  
  if [[ "$HANGS" -gt 0 ]]; then
    echo -e "${BOLD}⏸  Hangs:${NC}           ${RED}${HANGS}${NC}"
  else
    echo -e "${BOLD}⏸  Hangs:${NC}           ${GREEN}${HANGS}${NC}"
  fi
  
  echo ""
  echo -e "${BOLD}${CYAN}═══════════════════════════════════════════════════════════════${NC}"
  echo -e "📂 Run Dir: ${RUN_DIR}"
  echo -e "🔄 Updating every ${WATCH_INTERVAL}s... (Ctrl+C to exit)"
  echo ""
  
  # Queue preview
  QUEUE_DIR="${RUN_DIR%/fuzzer_stats}/queue"
  if [[ -d "$QUEUE_DIR" ]]; then
    QUEUE_SIZE=$(ls -1 "$QUEUE_DIR" | wc -l)
    echo -e "${BOLD}Recent Queue Entries:${NC}"
    ls -lth "$QUEUE_DIR" 2>/dev/null | head -6 | tail -5 | awk '{printf "  %s %s  %6s  %s\n", $6, $7, $5, $9}'
  fi
  
  # Wait for next update
  sleep "$WATCH_INTERVAL"
done
