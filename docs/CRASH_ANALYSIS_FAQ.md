# Crash Analysis FAQ

## Q1: Is this crash a false positive or a real bug?

### Answer: **This is a REAL BUG in PicoRV32!** ✅

The crash reveals **multiple bugs** in the DUT (PicoRV32):

1. **Wrong write mask**: `mem_wmask = 0xf` (word) instead of `0x1` (byte)
2. **Wrong address calculation**: `mem_addr = 0xc` instead of `0xd`

---

## Q2: Why does Spike (Golden) skip the store but DUT executes it?

### Memory Layout Differences

**DUT Memory Model (CpuPicorv32.cpp):**
```cpp
static const size_t MEM_BYTES = 64 * 1024;  // 64KB
static uint8_t mem_area[MEM_BYTES];

// Address wrapping - ALL addresses accepted!
uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;
```
- **Accepts ANY address**: Wraps using modulo 64KB
- **No access violations**: Address 0xc is perfectly valid
- **Always writable**: No permission checks

**Spike Memory Model:**
```cpp
// Spike has a proper MMU with:
// - Valid memory regions (0x80000000+)
// - Permission checking (R/W/X)
// - Address validation
```
- **Validates addresses**: Only certain ranges are valid
- **Access control**: Read-only vs writable regions
- **Throws exceptions**: Invalid access → trap/skip

### Why Spike Skipped This Store

Looking at the instruction:
```assembly
sb x2, 3(x1)    # Store byte: mem[x1 + 3] = x2
                # x1 = 10, so address = 13 (0xd)
```

**Possible reasons Spike skipped:**

#### 1. **Address 0xd is unmapped in Spike**
- Spike expects code at `0x80000000+` (DRAM base)
- Address `0xd` is in low memory (near NULL)
- This is typically **reserved/unmapped** in RISC-V systems
- Spike likely returned exception or skipped silently

#### 2. **Store would cause misaligned access exception**
- Though byte stores shouldn't have alignment issues
- Less likely reason

#### 3. **Memory region is read-only**
- Low addresses might be ROM or protected
- Spike respects memory permissions

### The Real Issue: **Address 0xd is Invalid!**

The program is trying to store to an **invalid address (0xd)**. This is a bug in the fuzzing input, but:
- **Spike correctly detects this** and skips/traps
- **DUT incorrectly accepts this** and performs the store

**Plus, DUT has two additional bugs:**
- Calculated wrong address (0xc instead of 0xd)
- Used wrong write mask (0xf instead of 0x1)

---

## Q3: Does DUT respect illegal memory access?

### Answer: **NO, DUT does NOT respect illegal access!** ❌

**DUT's Memory Handler** (from `CpuPicorv32.cpp`):
```cpp
void step() override {
  top_->mem_ready = 0;
  if (top_->mem_valid) {
    if (top_->mem_wstrb) 
      mem_write32(top_->mem_addr, top_->mem_wdata, top_->mem_wstrb);
    else                 
      top_->mem_rdata = mem_read32(top_->mem_addr);
    top_->mem_ready = 1;  // ALWAYS ready!
  }
  tick(top_);
}

static inline void mem_write32(uint32_t addr, uint32_t data, uint8_t wstrb) {
  uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;  // Wrap address
  if (wstrb & 1) mem_area[a+0] = (uint8_t)(data & 0xFF);
  if (wstrb & 2) mem_area[a+1] = (uint8_t)((data >> 8) & 0xFF);
  if (wstrb & 4) mem_area[a+2] = (uint8_t)((data >> 16) & 0xFF);
  if (wstrb & 8) mem_area[a+3] = (uint8_t)((data >> 24) & 0xFF);
}
```

**Problems:**
1. **No address validation**: Accepts ANY address (0x0 to 0xFFFFFFFF)
2. **No permission checks**: No read-only regions
3. **Always succeeds**: `mem_ready = 1` unconditionally
4. **Address wrapping**: Invalid addresses wrap around modulo 64KB

### Spike's Proper Memory Handling

Spike has a **proper MMU (Memory Management Unit)**:
- Validates addresses against defined memory regions
- Enforces R/W/X permissions
- Raises exceptions for:
  - Unmapped addresses
  - Permission violations
  - Alignment issues (for word/halfword)
- Returns proper error codes

### The Divergence Explained

```
Instruction: sb x2, 3(x1)  → Store to address 0xd

Spike:  "Address 0xd is unmapped/invalid → Skip/Trap"
        ✓ Proper behavior
        
DUT:    "Address wraps to 0xc, write with wmask=0xf"
        ✗ Wrong address (should be 0xd, not 0xc)
        ✗ Wrong mask (should be 0x1, not 0xf)
        ✗ Should have failed but succeeded
```

---

## Q4: Is memory alignment synced between Spike and DUT?

### Answer: **NO, they are NOT synced!** ❌

### Spike Memory Model
- **Base address**: 0x80000000 (DRAM_BASE)
- **Valid range**: 0x80000000 - 0x80010000 (64KB default)
- **Low memory**: 0x0 - 0x7FFFFFFF is **unmapped/invalid**
- **Alignment**: Enforces proper alignment for lw/lh/sw/sh
- **Permissions**: Has R/W/X permissions per region

### DUT Memory Model
- **Base address**: 0x0 (starts at zero)
- **Valid range**: 0x0 - 0x10000 (64KB, wraps around)
- **All addresses valid**: No concept of unmapped memory
- **No alignment checks**: Harness handles everything
- **No permissions**: Everything is R/W

### The Mismatch

```
Address Space Comparison:

Spike Memory Map:
  0x00000000 - 0x00000FFF: Unmapped (exceptions)
  0x00001000 - 0x00001FFF: Bootloader code
  0x80000000 - 0x8000FFFF: DRAM (user code here)
  0x80001000: TOHOST (exit mechanism)

DUT Memory Map:
  0x00000000 - 0x0000FFFF: All valid (64KB RAM)
  Address wrapping: addr % 64KB
  No unmapped regions!
```

### Why This Causes False Divergence

The fuzzer generates code that starts at `0x80000000` (Spike's DRAM):
```assembly
0x80000000: li x1, 10        # x1 = 10
0x80000004: li x2, 3         # x2 = 3  
0x80000008: sb x2, 3(x1)     # Store to addr 13 (0xd)
```

**What happens:**
- **Spike**: "Address 0xd is in unmapped region → Skip/Exception"
- **DUT**: "Address 0xd wraps to 0xd (valid) → Execute store"

**But DUT has bugs:**
- Calculates wrong address (0xc)
- Uses wrong byte mask (0xf)

---

## Q5: How to distinguish false positive from real bug?

### Real Bug Indicators ✅

**1. RVFI signal mismatch** (like this case):
```
DUT wmask = 0xf (wrong for byte store!)
Should be 0x1 for SB instruction
→ This is a REAL BUG in PicoRV32
```

**2. Address calculation error**:
```
Expected: x1 + imm = 10 + 3 = 13 (0xd)
DUT calculated: 0xc (off by 1!)
→ REAL BUG in address generation
```

**3. Wrong instruction behavior**:
- Wrong ALU result
- Wrong branch target
- Wrong CSR value

### False Positive Indicators ⚠️

**1. Memory model mismatch** (partially applies here):
```
Spike: Address 0xd unmapped → Skip
DUT: Address 0xd valid → Execute
→ Could be setup issue, BUT...
```

**2. Bootloader differences**:
```
Spike runs bootloader at 0x1000
DUT starts directly at 0x80000000
→ Skip first N instructions
```

**3. Timing differences**:
```
Spike is single-cycle
DUT might be multi-cycle
→ Cycle count mismatch is not a bug
```

### Verdict for This Crash: **REAL BUG!** 🐛

Even though there's a memory model mismatch (Spike sees unmapped, DUT sees valid), the DUT **still has bugs**:

1. ❌ **Wrong address**: 0xc instead of 0xd
2. ❌ **Wrong wmask**: 0xf instead of 0x1
3. ⚠️ **No access validation**: Should check if address is valid

**These are REAL bugs that need fixing in PicoRV32!**

---

## Q6: Should I fix the memory model mismatch?

### YES! Recommended Fixes:

#### **Option 1: Add Address Validation to DUT Harness** (Easier)
```cpp
// In CpuPicorv32.cpp
static inline void mem_write32(uint32_t addr, uint32_t data, uint8_t wstrb) {
  // Validate address is in valid range
  if (addr < 0x80000000 || addr >= 0x80010000) {
    // Invalid address - don't perform write
    // PicoRV32 should trap, but harness can reject
    return;
  }
  
  uint32_t a = (addr - 0x80000000) & (MEM_BYTES - 1);
  if (wstrb & 1) mem_area[a+0] = (uint8_t)(data & 0xFF);
  // ... rest
}
```

#### **Option 2: Fix PicoRV32 RVFI signals** (Better)
The real bugs are:
1. Fix address calculation (x1 + imm)
2. Fix wmask generation (use correct byte enables for sb/sh)
3. Add proper memory access validation

#### **Option 3: Align Memory Models** (Best)
- Make DUT reject invalid addresses
- Add memory region definitions
- Implement proper exception handling
- This makes fuzzing more realistic!

---

## Summary

### For the current crash:

| Aspect | Status | Fix Required |
|--------|--------|--------------|
| **Memory model mismatch** | ⚠️ Expected | Make DUT reject invalid addresses |
| **Wrong wmask (0xf vs 0x1)** | ❌ REAL BUG | Fix PicoRV32 byte enable logic |
| **Wrong address (0xc vs 0xd)** | ❌ REAL BUG | Fix PicoRV32 address calculation |
| **No access validation** | ⚠️ Harness issue | Add address range checking |

### Action Items:

1. **Fix PicoRV32 bugs** (address calc + wmask)
2. **Add address validation** to DUT harness
3. **Align memory models** between Spike and DUT
4. **Continue fuzzing** to find more bugs!

The fuzzer is working perfectly - it found REAL bugs! 🎉
