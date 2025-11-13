#!/bin/bash
# Comprehensive crash analysis tool for differential testing bugs

set -e

CRASH_LOG="$1"

if [ -z "$CRASH_LOG" ]; then
    echo "Usage: $0 <crash_log_file>"
    echo
    echo "Example:"
    echo "  $0 workdir/logs/crash/crash_golden_divergence_mem_kind_20251112T223904_cyc11.log"
    exit 1
fi

if [ ! -f "$CRASH_LOG" ]; then
    echo "Error: Crash log not found: $CRASH_LOG"
    exit 1
fi

echo "============================================"
echo "CRASH ANALYSIS REPORT"
echo "============================================"
echo "File: $CRASH_LOG"
echo

# Extract key information
REASON=$(grep "^Reason:" "$CRASH_LOG" | cut -d: -f2- | xargs)
CYCLE=$(grep "^Cycle:" "$CRASH_LOG" | cut -d: -f2 | xargs)
PC=$(grep "^PC:" "$CRASH_LOG" | cut -d: -f2 | xargs)
INSN=$(grep "^Instruction:" "$CRASH_LOG" | cut -d: -f2 | xargs)

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. CRASH SUMMARY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Reason:      $REASON"
echo "  Cycle:       $CYCLE"
echo "  PC:          $PC"
echo "  Instruction: $INSN"
echo

# Decode instruction
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. INSTRUCTION ANALYSIS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Extract disassembly - look for lines after "Disassembly of section" until "Details:"
DISASM=$(sed -n '/^Disassembly of section/,/^Details:/p' "$CRASH_LOG" | \
         grep -E '^\s*[0-9a-f]+:' | grep -v "^--$" || echo "")

if [ -n "$DISASM" ]; then
    echo "Disassembly:"
    echo "$DISASM" | sed 's/^/  /'
else
    echo "  (Disassembly not found in log)"
fi
echo

# Decode instruction fields
echo "Instruction encoding: $INSN"
OPCODE=$(python3 -c "print(f'0b{int('$INSN', 16) & 0x7f:07b}')" 2>/dev/null || echo "unknown")
echo "  Opcode: $OPCODE"

# Identify instruction type
case "$REASON" in
    *mem*|*load*|*store*)
        echo "  Type: MEMORY OPERATION"
        echo "  → Check: load/store unit, memory alignment, byte enables"
        ;;
    *branch*|*jump*)
        echo "  Type: CONTROL FLOW"
        echo "  → Check: branch predictor, jump logic, PC calculation"
        ;;
    *alu*|*arithmetic*)
        echo "  Type: ARITHMETIC/LOGIC"
        echo "  → Check: ALU implementation, flag computation"
        ;;
    *divergence*)
        echo "  Type: GOLDEN MODEL MISMATCH"
        echo "  → DUT behavior differs from Spike (golden reference)"
        ;;
esac
echo

# Extract divergence details
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. DIVERGENCE DETAILS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if grep -q "^Details:" "$CRASH_LOG"; then
    sed -n '/^Details:/,/^=== DUT/p' "$CRASH_LOG" | \
        grep -v "^===" | sed 's/^/  /'
else
    echo "  (No divergence details found)"
fi
echo

# Compare traces side by side
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. EXECUTION TRACE COMPARISON"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Starting from PC 0x80000000 (user code start)"
echo

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Extract traces starting from 0x80000000
DUT_FROM_USER=$(sed -n '/^=== DUT Execution Trace/,/^=== Golden/p' "$CRASH_LOG" | \
                grep "^0x80000000" -A 100 || echo "")
GOLD_FROM_USER=$(sed -n '/^=== Golden Model Trace/,$p' "$CRASH_LOG" | \
                 grep "^0x80000000" -A 100 || echo "")

if [ -n "$DUT_FROM_USER" ] && [ -n "$GOLD_FROM_USER" ]; then
    echo -e "${BOLD}Trace Format:${NC} ${CYAN}pc_r${NC},${CYAN}pc_w${NC},${YELLOW}insn${NC},${GREEN}rd_addr${NC},${GREEN}rd_wdata${NC},${BLUE}mem_addr${NC},${RED}mem_rmask${NC},${RED}mem_wmask${NC},trap"
    echo -e "  ${CYAN}pc_r/pc_w${NC}=Program Counter (read/write), ${YELLOW}insn${NC}=Instruction, ${GREEN}rd_addr/rd_wdata${NC}=Register destination, ${BLUE}mem_addr${NC}=Memory address, ${RED}mem_rmask/wmask${NC}=Read/Write mask"
    echo
    echo "Side-by-side comparison (DUT | GOLDEN):"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Create temporary files for side-by-side comparison
    TMP_DUT=$(mktemp)
    TMP_GOLD=$(mktemp)
    
    echo "$DUT_FROM_USER" > "$TMP_DUT"
    echo "$GOLD_FROM_USER" > "$TMP_GOLD"
    
    # Get line counts
    DUT_LINES=$(wc -l < "$TMP_DUT")
    GOLD_LINES=$(wc -l < "$TMP_GOLD")
    MAX_LINES=$((DUT_LINES > GOLD_LINES ? DUT_LINES : GOLD_LINES))
    
    # Print header
    printf "${BOLD}%-80s | %-80s${NC}\n" "DUT" "GOLDEN"
    printf "%-80s | %-80s\n" "$(head -c 80 < /dev/zero | tr '\0' '-')" "$(head -c 80 < /dev/zero | tr '\0' '-')"
    
    # Print side by side
    for i in $(seq 1 $MAX_LINES); do
        DUT_LINE=$(sed -n "${i}p" "$TMP_DUT")
        GOLD_LINE=$(sed -n "${i}p" "$TMP_GOLD")
        
        # Skip empty lines and section markers
        if [[ -z "$DUT_LINE" && -z "$GOLD_LINE" ]] || [[ "$DUT_LINE" =~ ^=== ]] || [[ "$GOLD_LINE" =~ ^=== ]]; then
            continue
        fi
        
        # Highlight differences
        if [ "$DUT_LINE" = "$GOLD_LINE" ]; then
            # Same - green checkmark
            printf "${GREEN}✓${NC} %-78s | ${GREEN}✓${NC} %-78s\n" "$DUT_LINE" "$GOLD_LINE"
        else
            # Different - red highlight with detailed field comparison
            printf "${RED}✗${NC} %-78s ${RED}│${NC} ${RED}✗${NC} %-78s\n" "$DUT_LINE" "$GOLD_LINE"
            
            # Parse and highlight which fields differ
            if [[ "$DUT_LINE" =~ ^0x && "$GOLD_LINE" =~ ^0x ]]; then
                IFS=',' read -ra DUT_FIELDS <<< "$DUT_LINE"
                IFS=',' read -ra GOLD_FIELDS <<< "$GOLD_LINE"
                
                DIFF_FIELDS=""
                [ "${DUT_FIELDS[0]}" != "${GOLD_FIELDS[0]}" ] && DIFF_FIELDS="${DIFF_FIELDS}pc_r "
                [ "${DUT_FIELDS[1]}" != "${GOLD_FIELDS[1]}" ] && DIFF_FIELDS="${DIFF_FIELDS}pc_w "
                [ "${DUT_FIELDS[2]}" != "${GOLD_FIELDS[2]}" ] && DIFF_FIELDS="${DIFF_FIELDS}insn "
                [ "${DUT_FIELDS[3]}" != "${GOLD_FIELDS[3]}" ] && DIFF_FIELDS="${DIFF_FIELDS}${GREEN}rd_addr${NC} "
                [ "${DUT_FIELDS[4]}" != "${GOLD_FIELDS[4]}" ] && DIFF_FIELDS="${DIFF_FIELDS}${GREEN}rd_wdata${NC} "
                [ "${DUT_FIELDS[5]}" != "${GOLD_FIELDS[5]}" ] && DIFF_FIELDS="${DIFF_FIELDS}${BLUE}mem_addr${NC} "
                [ "${DUT_FIELDS[6]}" != "${GOLD_FIELDS[6]}" ] && DIFF_FIELDS="${DIFF_FIELDS}${RED}mem_rmask${NC} "
                [ "${DUT_FIELDS[7]}" != "${GOLD_FIELDS[7]}" ] && DIFF_FIELDS="${DIFF_FIELDS}${RED}mem_wmask${NC} "
                
                if [ -n "$DIFF_FIELDS" ]; then
                    echo -e "  ${YELLOW}→ Differs in:${NC} $DIFF_FIELDS"
                fi
            fi
        fi
    done
    
    rm -f "$TMP_DUT" "$TMP_GOLD"
else
    echo "DUT (Starting from 0x80000000):"
    echo "$DUT_FROM_USER" | head -10 | sed 's/^/  /'
    echo
    echo "Golden Model (Starting from 0x80000000):"
    echo "$GOLD_FROM_USER" | head -10 | sed 's/^/  /'
fi
echo

# Highlight differences
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. KEY DIFFERENCES"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Parse DUT and Golden traces
DUT_LAST=$(sed -n '/^=== DUT Execution Trace/,/^=== Golden/p' "$CRASH_LOG" | \
           grep "^0x" | tail -1)
GOLD_LAST=$(sed -n '/^=== Golden Model Trace/,$p' "$CRASH_LOG" | \
            grep "^0x" | tail -1)

if [ -n "$DUT_LAST" ] && [ -n "$GOLD_LAST" ]; then
    DUT_PC=$(echo "$DUT_LAST" | cut -d, -f1)
    GOLD_PC=$(echo "$GOLD_LAST" | cut -d, -f1)
    
    DUT_INSN=$(echo "$DUT_LAST" | cut -d, -f3)
    GOLD_INSN=$(echo "$GOLD_LAST" | cut -d, -f3)
    
    DUT_RD=$(echo "$DUT_LAST" | cut -d, -f4)
    GOLD_RD=$(echo "$GOLD_LAST" | cut -d, -f4)
    
    DUT_WDATA=$(echo "$DUT_LAST" | cut -d, -f5)
    GOLD_WDATA=$(echo "$GOLD_LAST" | cut -d, -f5)
    
    DUT_MEM=$(echo "$DUT_LAST" | cut -d, -f6)
    GOLD_MEM=$(echo "$GOLD_LAST" | cut -d, -f6)
    
    DUT_RMASK=$(echo "$DUT_LAST" | cut -d, -f7)
    GOLD_RMASK=$(echo "$GOLD_LAST" | cut -d, -f7)
    
    DUT_WMASK=$(echo "$DUT_LAST" | cut -d, -f8)
    GOLD_WMASK=$(echo "$GOLD_LAST" | cut -d, -f8)
    
    echo "Field Comparison:"
    echo "  PC:       DUT=$DUT_PC  GOLD=$GOLD_PC"
    [ "$DUT_PC" != "$GOLD_PC" ] && echo "    ⚠️  PC MISMATCH! Branch/jump error?"
    
    echo "  Insn:     DUT=$DUT_INSN  GOLD=$GOLD_INSN"
    [ "$DUT_INSN" != "$GOLD_INSN" ] && echo "    ⚠️  INSTRUCTION MISMATCH! Fetch error?"
    
    echo "  rd_addr:  DUT=$DUT_RD  GOLD=$GOLD_RD"
    [ "$DUT_RD" != "$GOLD_RD" ] && echo "    ⚠️  DESTINATION REGISTER MISMATCH!"
    
    echo "  rd_wdata: DUT=$DUT_WDATA  GOLD=$GOLD_WDATA"
    [ "$DUT_WDATA" != "$GOLD_WDATA" ] && echo "    ⚠️  WRITE DATA MISMATCH! ALU error?"
    
    echo "  mem_addr: DUT=$DUT_MEM  GOLD=$GOLD_MEM"
    [ "$DUT_MEM" != "$GOLD_MEM" ] && echo "    ⚠️  MEMORY ADDRESS MISMATCH! Address calc error?"
    
    echo "  mem_rmask: DUT=$DUT_RMASK  GOLD=$GOLD_RMASK"
    [ "$DUT_RMASK" != "$GOLD_RMASK" ] && echo "    ⚠️  READ MASK MISMATCH! Load unit error?"
    
    echo "  mem_wmask: DUT=$DUT_WMASK  GOLD=$GOLD_WMASK"
    [ "$DUT_WMASK" != "$GOLD_WMASK" ] && echo "    ⚠️  WRITE MASK MISMATCH! Store unit error?"
else
    echo "  (Unable to parse trace data)"
fi
echo

# Provide debugging hints
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6. DUT MODULE ANALYSIS"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

case "$REASON" in
    *mem_kind*|*mem_rmask*|*mem_wmask*)
        echo "📍 SUSPECTED MODULE: Load/Store Unit (LSU)"
        echo ""
        echo "Likely causes:"
        echo "  • Incorrect byte enable generation"
        echo "  • Wrong mem_rmask/mem_wmask signals"
        echo "  • Load/store decoder error"
        echo ""
        echo "Where to look in DUT:"
        echo "  → picorv32.v: memory interface logic"
        echo "  → Check: rvfi_mem_rmask, rvfi_mem_wmask generation"
        echo "  → Check: Load/store instruction decode"
        echo "  → Check: Byte/halfword/word size selection"
        ;;
        
    *mem_addr*)
        echo "📍 SUSPECTED MODULE: Address Generation Unit (AGU)"
        echo ""
        echo "Likely causes:"
        echo "  • Incorrect address calculation"
        echo "  • Wrong base+offset addition"
        echo "  • Sign extension error in immediate"
        echo ""
        echo "Where to look in DUT:"
        echo "  → Check: mem_addr = rs1 + imm calculation"
        echo "  → Check: Immediate sign extension"
        ;;
        
    *mem_wdata*|*mem_rdata*)
        echo "📍 SUSPECTED MODULE: Memory Data Path"
        echo ""
        echo "Likely causes:"
        echo "  • Data alignment error"
        echo "  • Byte lane selection wrong"
        echo "  • Sign/zero extension error on loads"
        echo ""
        echo "Where to look in DUT:"
        echo "  → Check: Load data sign/zero extension"
        echo "  → Check: Store data byte positioning"
        ;;
        
    *rd_wdata*)
        echo "📍 SUSPECTED MODULE: ALU / Execution Unit"
        echo ""
        echo "Likely causes:"
        echo "  • Arithmetic operation error"
        echo "  • Logic operation error"
        echo "  • Shifter error"
        echo ""
        echo "Where to look in DUT:"
        echo "  → Check: ALU operation for this instruction"
        echo "  → Check: Result multiplexing"
        echo "  → Check: Immediate handling"
        ;;
        
    *pc_w*|*pc_r*)
        echo "📍 SUSPECTED MODULE: Program Counter (PC) / Branch Unit"
        echo ""
        echo "Likely causes:"
        echo "  • Branch target calculation error"
        echo "  • Jump target error"
        echo "  • PC increment error"
        echo ""
        echo "Where to look in DUT:"
        echo "  → Check: Branch condition evaluation"
        echo "  → Check: Jump target calculation"
        echo "  → Check: PC = PC + 4 logic"
        ;;
        
    *)
        echo "📍 SUSPECTED MODULE: Unknown (check details above)"
        ;;
esac
echo

# Replay instructions
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7. REPRODUCTION"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

CRASH_BIN="${CRASH_LOG%.log}.bin"
if [ -f "$CRASH_BIN" ]; then
    echo "Reproduce with:"
    echo "  ./tools/run_once.sh $CRASH_BIN"
    echo
    echo "Debug with waveforms:"
    echo "  1. Enable tracing in rtl/cpu/picorv32/"
    echo "  2. Run: ./tools/run_once.sh $CRASH_BIN"
    echo "  3. View: gtkwave traces/waveform.vcd"
    echo "  4. Look at cycle $CYCLE, PC $PC"
else
    echo "  Binary not found: $CRASH_BIN"
fi
echo

echo "============================================"
echo "NEXT STEPS"
echo "============================================"
echo "1. Review the DUT module identified above"
echo "2. Check signals at cycle $CYCLE"
echo "3. Compare with Spike's expected behavior"
echo "4. Fix the bug in RTL"
echo "5. Rerun fuzzer to verify fix"
echo
