# Feedback Verification and Crash Analysis Guide

## How to Know if Feedback is Working

### 1. **Check AFL++ Output During Fuzzing**

When you run the fuzzer, watch these indicators in the AFL++ UI:

```bash
./run_with_ui.sh
```

**Key metrics that show feedback is working:**

| Metric | What to Look For | Meaning |
|--------|-----------------|---------|
| **map density** | Increases over time (e.g., 0.01% → 0.5% → 2.0%) | New code paths being discovered |
| **new edges on** | Regular increments | Fuzzer finding new execution paths |
| **corpus count** | Growing number | AFL++ adding interesting inputs |
| **pending faves** | Eventually decreases | Fuzzer exploring new paths efficiently |
| **stability** | 90-100% | Execution is deterministic (good!) |

**Example of GOOD feedback:**
```
     map density : 1.45% / 3.21%
    new edges on : 1234 (last saved path: 45 sec ago)
   corpus count  : 156 (45 favs, 12 pending)
```

**Example of BAD feedback (not working):**
```
     map density : 0.00% / 0.00%  ← STUCK!
    new edges on : 0 (never)        ← NO NEW PATHS!
   corpus count  : 8 (8 favs, 0 pending) ← NOT GROWING!
```

### 2. **Check the Queue Directory**

```bash
# Should see new files being added over time
ls -lh workdir/queue/ | wc -l

# Watch it grow
watch -n 5 'ls -lh workdir/queue/ | wc -l'
```

If the queue grows from 8 → 50 → 200+ files, feedback is working!

### 3. **Check for AFL++ Instrumentation**

```bash
# Verify AFL++ symbols are compiled in
nm afl/afl_picorv32 | grep __afl_
```

Should show:
```
__afl_already_initialized_early
__afl_already_initialized_forkserver
__afl_area_ptr
__afl_prev_loc
```

### 4. **Check Feedback Code is Compiled**

```bash
# Verify DUT feedback functions exist
nm afl/afl_picorv32 | grep -i feedback
```

Should show:
```
Feedback::report_instruction(...)
Feedback::init(...)
```

### 5. **Monitor Coverage Output**

With Verilator coverage enabled (now default!), check logs and coverage files:

```bash
# Check if coverage is enabled in logs
tail -f workdir/logs/runtime.log | grep -i coverage

# Look for coverage data files
ls -lh workdir/traces/coverage.dat

# Analyze coverage with Verilator tools (if available)
verilator_coverage --annotate coverage_html workdir/traces/coverage.dat
```

Should see in logs:
```
[COVERAGE] Verilator structural coverage enabled
[COVERAGE] Output file: workdir/traces/coverage.dat
[COVERAGE] Metrics: line, toggle, FSM, trace
```

**Coverage file location:** `workdir/traces/coverage.dat` (written after each test case)

---

## Your Current Fuzzer Feedback Status

### ✅ **DUT Hardware Feedback: ENABLED**

Your harness implements **dual feedback mechanisms**:

#### 1. **DUT Execution Feedback** (via RVFI signals)
   - **What:** Tracks actual hardware execution paths
   - **How:** Hashes PC, register writes, memory accesses
   - **Code:** `afl_harness/src/Feedback.cpp::report_instruction()`
   - **Active:** Yes, always on
   - **Output:** Updates `__afl_area_ptr[]` bitmap with:
     - PC edge transitions
     - Memory access patterns
     - Register write patterns

#### 2. **Verilator Structural Coverage** (RTL-level)
   - **What:** Tracks RTL signal toggles, line coverage
   - **How:** Verilator built-in coverage instrumentation
   - **Code:** `afl_harness/src/VerilatorCoverage.cpp`
   - **Active:** ✅ **NOW ENABLED BY DEFAULT** (you just enabled it!)
   - **Output:** Writes `.dat` files with:
     - Line coverage (which RTL lines executed)
     - Toggle coverage (which signals changed)
     - FSM coverage (which states visited)

### 🔧 **How to Verify It's Working**

```bash
# 1. Rebuild with coverage enabled
make clean
make fuzz-build

# 2. Check compilation included coverage
grep VM_COVERAGE afl/Makefile
# Should show: CXXFLAGS += -DVM_COVERAGE=1

# 3. Run a short fuzzing session
timeout 60 ./tools/run_fuzz.sh

# 4. Check if AFL++ is finding paths
ls -lh workdir/queue/
# Should see more than just the initial 8 seeds

# 5. Check coverage output
ls -lh workdir/coverage*.dat
# Should exist if Verilator coverage is working
```

---

## How to Analyze Crashes

### Quick Analysis Tool

Use the automated crash analyzer:

```bash
./tools/analyze_crash.sh workdir/logs/crash/<crash_file>.log
```

**Example output:**
```
============================================
CRASH ANALYSIS REPORT
============================================

1. CRASH SUMMARY
  Reason:      golden_divergence_mem_kind
  Cycle:       11
  PC:          0x80000004
  Instruction: 0x00300103 (lb x2, 3(x0))

2. INSTRUCTION ANALYSIS
  Type: MEMORY OPERATION
  → Check: load/store unit, memory alignment, byte enables

3. DIVERGENCE DETAILS
  DUT:  load=1 store=0 addr=0x0 mem_rmask=0xf
  GOLD: load=0 store=0 addr=0x0 mem_rmask=0x0
  
  ⚠️  DUT performed load, Golden did NOT!

4. KEY DIFFERENCES
  mem_rmask: DUT=0xf  GOLD=0x0
    ⚠️  READ MASK MISMATCH! Load unit error?

5. DUT MODULE ANALYSIS
  📍 SUSPECTED MODULE: Load/Store Unit (LSU)
  
  Where to look in DUT:
    → rtl/cpu/picorv32/picorv32.v: memory interface logic
    → Check: rvfi_mem_rmask generation
    → Check: Load instruction decode
```

### Manual Crash Analysis

#### Step 1: Read the Crash Log

```bash
cat workdir/logs/crash/crash_*.log
```

Look for:
- **Reason**: Type of mismatch (mem_kind, regfile, pc, etc.)
- **Cycle**: When divergence occurred
- **PC**: Address of the instruction
- **Instruction**: Encoding (e.g., 0x00300103)

#### Step 2: Understand the Divergence

| Divergence Type | Meaning | Likely DUT Bug |
|----------------|---------|----------------|
| `mem_kind` | DUT did load/store when it shouldn't (or vice versa) | Load/store decoder error |
| `mem_rmask` | Read byte enable mismatch | Load unit byte select wrong |
| `mem_wmask` | Write byte enable mismatch | Store unit byte select wrong |
| `mem_addr` | Memory address different | Address generation unit (AGU) error |
| `mem_wdata` | Store data different | Store data path error |
| `regfile` | Register write value mismatch | ALU computation error |
| `pc_w` | Next PC different | Branch/jump calculation error |

#### Step 3: Compare Traces

The crash log includes the last 100 instructions from both DUT and Golden:

**Trace format:**
```
pc_r, pc_w, insn, rd_addr, rd_wdata, mem_addr, mem_rmask, mem_wmask, trap
```

**Example:**
```
DUT:   0x80000004,0x80000008,0x00300103,2,0x00000000,0x00000000,0xf,0x0,0
GOLD:  0x80000004,0x80000008,0x00300103,0,0x00000000,0x00000000,0x0,0x0,0
       ^^^^^^^^^^^ same PC     ^^^^^^^^ same instruction
                                        ^ rd_addr different (2 vs 0)
                                                             ^^^ mem_rmask different!
```

This shows:
- Same instruction executed (`0x00300103` = `lb x2, 3(x0)`)
- DUT wrote to `rd=2` (x2), Golden wrote to `rd=0` (x0/zero)
- DUT set `mem_rmask=0xf` (tried to load), Golden set `0x0` (no load)
- **Bug:** DUT incorrectly executed a load when it shouldn't

#### Step 4: Decode the Instruction

```bash
# Disassemble the instruction
riscv32-unknown-elf-objdump -D -b binary -m riscv:rv32 <crash>.bin
```

Or use online RISC-V decoder with the hex value.

**Example:** `0x00300103`
- Opcode: `0000011` = LOAD
- funct3: `000` = LB (load byte)
- rd: `00010` = x2
- rs1: `00000` = x0
- imm: `0x003` = 3
- Disassembly: `lb x2, 3(x0)` = Load byte from address (x0 + 3) into x2

#### Step 5: Identify the DUT Module

Based on the divergence type:

| Divergence | DUT Module | What to Check |
|-----------|-----------|--------------|
| `mem_*` | Load/Store Unit (LSU) | `picorv32.v`: memory interface, byte enables, `rvfi_mem_*` signals |
| `regfile` | ALU / Execution Unit | `picorv32.v`: ALU operations, result mux |
| `pc_*` | Branch/Jump Unit | `picorv32.v`: branch condition, PC calculation |
| `insn` | Instruction Fetch | `picorv32.v`: instruction memory interface |

#### Step 6: Reproduce with Waveforms

```bash
# Run the crashing input once
./tools/run_once.sh workdir/logs/crash/crash_*.bin

# View waveforms (if you enable VCD tracing in Verilator)
gtkwave workdir/traces/waveform.vcd
```

In GTKWave:
1. Find cycle 11 (from crash log)
2. Look at PC = 0x80000004
3. Check: `rvfi_mem_rmask`, `rvfi_rd_addr`, memory interface signals
4. Compare with expected behavior from Spike

---

## Example: Analyzing Your Current Crash

**Crash:** `crash_golden_divergence_mem_kind_20251112T223904_cyc11.log`

### What Happened?

```
Instruction: lb x2, 3(x0)  (load byte from address 3)
Cycle: 11
PC: 0x80000004

DUT behavior:
  - Executed a LOAD (mem_rmask=0xf)
  - Wrote to x2 (rd_addr=2)
  - Read from address 0x0

Golden behavior:
  - Did NOT execute a load (mem_rmask=0x0)
  - Did NOT write to any register (rd_addr=0)
```

### Root Cause Analysis

**Problem:** DUT executed a load instruction, but Spike (Golden) did NOT.

**Why might Golden skip the instruction?**
1. Instruction at 0x80000004 might be invalid in Golden's view
2. Golden might have trapped before reaching this instruction
3. Golden's memory might have different content at 0x80000004

**Likely DUT Bug:**
- PicoRV32 is incorrectly decoding or executing a load
- Check if the instruction should have trapped (e.g., invalid address 0x0)
- Check if PicoRV32's memory interface is correctly handling address 0x0

### Where to Look in RTL

**File:** `rtl/cpu/picorv32/picorv32.v`

**Signals to check:**
```verilog
rvfi_valid      // Should be 1 when instruction completes
rvfi_insn       // Should be 0x00300103
rvfi_mem_rmask  // DUT shows 0xf, should be 0x0
rvfi_rd_addr    // DUT shows 2, should be 0
rvfi_rd_wdata   // Register write data
```

**What to verify:**
1. Is the load instruction decoder working correctly?
2. Is `mem_valid` being asserted incorrectly?
3. Is there a spurious load signal being generated?
4. Does address 0x0 have special handling that's broken?

### Fix Workflow

1. Identify the bug in `picorv32.v`
2. Fix the RTL
3. Rebuild: `make clean && make fuzz-build`
4. Test the fix: `./tools/run_once.sh workdir/logs/crash/crash_*.bin`
5. Verify no crash: Should exit cleanly
6. Rerun fuzzer: `./tools/run_fuzz.sh`
7. Monitor for new crashes (different bugs)

---

## Summary

### ✅ Feedback is Working If:
- Map density increases over time
- New edges are regularly found
- Queue directory grows
- Coverage files are written

### ✅ How to Analyze Crashes:
1. Use `./tools/analyze_crash.sh <log_file>`
2. Compare DUT vs Golden traces
3. Identify which DUT module has the bug
4. Reproduce with `./tools/run_once.sh <bin_file>`
5. Fix the RTL bug
6. Verify fix and continue fuzzing

### ✅ Your Current Status:
- **DUT Feedback:** ✅ Enabled (always on)
- **Verilator Coverage:** ✅ Enabled (just enabled by default)
- **Crash Analysis Tool:** ✅ Created (`tools/analyze_crash.sh`)
- **Ready to Fuzz:** ⚠️ HEAD commit has bugs (expected during fuzzing!)

**Next steps:** Fix the crash you found, then continue fuzzing to find more bugs! 🐛🔍
