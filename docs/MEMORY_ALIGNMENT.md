# Memory Model Alignment Guide

## Overview

This guide explains how to align the memory models between Spike (golden model) and the DUT to reduce false positives and ensure realistic fuzzing.

## Problem: Misaligned Memory Models

### Before Alignment

**DUT Memory Model (OLD):**
```cpp
// Memory starts at address 0x0
static const size_t MEM_BYTES = 64 * 1024;
static uint8_t mem_area[MEM_BYTES];

// Address wrapping - accepts ANY address
uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;
```
- ❌ Base address: `0x0` (doesn't match Spike)
- ❌ All addresses valid (wraps modulo 64KB)
- ❌ No unmapped regions
- ❌ Always succeeds (`mem_ready = 1`)

**Spike Memory Model:**
```
Address Space:
  0x00000000 - 0x00000FFF: Unmapped (causes exceptions)
  0x00001000 - 0x00001FFF: Bootloader region
  0x80000000 - 0x8000FFFF: DRAM (user code executes here)
  0x80010000+:             Unmapped
```
- ✅ Base address: `0x80000000` (RISC-V convention)
- ✅ Defined valid ranges
- ✅ Unmapped regions cause exceptions
- ✅ Proper error handling

### After Alignment

**DUT Memory Model (NEW):**
```cpp
// Memory layout aligned with Spike
static const uint32_t MEM_BASE = 0x80000000;
static const size_t MEM_BYTES = 64 * 1024;  // 64 KB
static uint8_t mem_area[MEM_BYTES];

// Valid range: 0x80000000 - 0x8000FFFF
static inline bool is_valid_addr(uint32_t addr) {
  return (addr >= MEM_BASE) && (addr < (MEM_BASE + MEM_BYTES));
}
```
- ✅ Base address: `0x80000000` (matches Spike)
- ✅ Address validation
- ✅ Unmapped regions rejected
- ✅ Realistic behavior

## Changes Made

### 1. Updated Memory Base Address

**File:** `afl_harness/src/CpuPicorv32.cpp`

**Before:**
```cpp
static const size_t MEM_BYTES = 64 * 1024;
static uint8_t mem_area[MEM_BYTES];

static inline uint32_t mem_read32(uint32_t addr) {
  uint32_t a = (addr & (MEM_BYTES - 1)) & ~0x3u;  // Wrapping
  return mem_area[a] | (mem_area[a+1] << 8) | ...;
}
```

**After:**
```cpp
static const uint32_t MEM_BASE = 0x80000000;
static const size_t MEM_BYTES = 64 * 1024;
static uint8_t mem_area[MEM_BYTES];

static inline bool is_valid_addr(uint32_t addr) {
  return (addr >= MEM_BASE) && (addr < (MEM_BASE + MEM_BYTES));
}

static inline uint32_t addr_to_offset(uint32_t addr) {
  return (addr - MEM_BASE) & ~0x3u;
}

static inline uint32_t mem_read32(uint32_t addr) {
  if (!is_valid_addr(addr)) {
    return 0;  // Unmapped read returns 0
  }
  uint32_t offset = addr_to_offset(addr);
  return mem_area[offset] | (mem_area[offset+1] << 8) | ...;
}
```

### 2. Added Address Validation

**Memory Write:**
```cpp
static inline void mem_write32(uint32_t addr, uint32_t data, uint8_t wstrb) {
  // Validate address is in mapped region
  if (!is_valid_addr(addr)) {
    // Silently ignore writes to unmapped addresses
    // This matches Spike's behavior of skipping invalid stores
    return;
  }
  
  uint32_t offset = addr_to_offset(addr);
  if (wstrb & 1) mem_area[offset+0] = (uint8_t)(data & 0xFF);
  // ... rest of write
}
```

### 3. Updated Memory Interface

**Bus Access Handling:**
```cpp
void step() override {
  top_->mem_ready = 0;
  if (top_->mem_valid) {
    if (is_valid_addr(top_->mem_addr)) {
      // Valid address - service the request
      if (top_->mem_wstrb) {
        mem_write32(top_->mem_addr, top_->mem_wdata, top_->mem_wstrb);
      } else {
        top_->mem_rdata = mem_read32(top_->mem_addr);
      }
      top_->mem_ready = 1;
    } else {
      // Invalid address - don't respond (bus timeout)
      top_->mem_ready = 0;
    }
  }
  tick(top_);
}
```

## Benefits of Alignment

### 1. Eliminates False Positives

**Before:**
```
Test: Store to address 0xd
Spike: "Unmapped address → Skip/Exception"
DUT:   "Address wraps to 0xd → Execute"
Result: False divergence (memory model mismatch)
```

**After:**
```
Test: Store to address 0xd
Spike: "Unmapped address → Skip/Exception"
DUT:   "Unmapped address → Skip (mem_ready=0)"
Result: No divergence (both reject invalid access)
```

### 2. Exposes Real Bugs

Now divergences are **always real bugs**:
- ❌ Wrong address calculation
- ❌ Wrong byte enables
- ❌ Wrong ALU results
- ❌ Wrong branch targets

### 3. Realistic Fuzzing

The DUT now behaves like a real system:
- ✅ Has defined memory map
- ✅ Rejects invalid addresses
- ✅ Matches processor specifications
- ✅ Tests real-world scenarios

## Memory Map

After alignment, both models use:

```
┌─────────────────────────────────────┐
│ 0x00000000 - 0x7FFFFFFF            │
│ UNMAPPED                            │
│ (Access causes no response)         │
├─────────────────────────────────────┤
│ 0x80000000 - 0x8000FFFF            │
│ DRAM (64 KB)                        │
│ - User code execution               │
│ - Stack and heap                    │
│ - Fuzzer input loaded here          │
├─────────────────────────────────────┤
│ 0x80010000 - 0x80FFFFFF            │
│ UNMAPPED                            │
│ (Future expansion)                  │
├─────────────────────────────────────┤
│ 0x81000000 - 0xFFFFFFFF            │
│ UNMAPPED                            │
│ (Peripherals could be mapped here)  │
└─────────────────────────────────────┘
```

### Special Addresses

- **0x80000000**: Program entry point (where fuzzer loads code)
- **0x80001000**: TOHOST address (exit mechanism via `sw x6, 0(x5)`)
- **0x8000FFFF**: End of DRAM

## Testing the Changes

### 1. Rebuild the Fuzzer

```bash
make clean
make fuzz-build
```

### 2. Test with Invalid Address

Create a test that accesses unmapped memory:

```assembly
# test_unmapped.s
li x1, 10
sb x2, 3(x1)    # Store to 0xd (unmapped)
# Both DUT and Spike should skip/timeout
```

**Expected result:** Both models skip the store, no divergence.

### 3. Test with Valid Address

```assembly
# test_valid.s
lui x1, 0x80000    # x1 = 0x80000000
addi x1, x1, 0x100 # x1 = 0x80000100
li x2, 42
sb x2, 0(x1)       # Store to 0x80000100 (valid)
# Both models should execute
```

**Expected result:** Both models execute the store successfully.

## Impact on Existing Crashes

### Previous Crash: `golden_divergence_mem_kind`

**Before alignment:**
```
Reason: golden_divergence_mem_kind
Details:
  DUT: store=1 addr=0xc (wraps to valid)
  GOLD: store=0 addr=0x0 (unmapped, skipped)
Status: False positive (memory model mismatch)
```

**After alignment:**
```
Reason: No crash (both skip invalid address)
OR
Reason: golden_divergence_mem_kind (if DUT has bugs)
Details:
  DUT: store=1 addr=0xc wmask=0xf (BUG: wrong mask)
  GOLD: store=0 addr=0x0 (unmapped)
Status: REAL BUG (DUT shouldn't access unmapped addr)
```

## Advanced: Adding Peripheral Regions

If you want to add peripherals (UART, GPIO, etc.):

```cpp
// Memory map with peripherals
static const uint32_t DRAM_BASE = 0x80000000;
static const uint32_t DRAM_SIZE = 64 * 1024;

static const uint32_t UART_BASE = 0x10000000;
static const uint32_t UART_SIZE = 0x1000;

static const uint32_t GPIO_BASE = 0x10001000;
static const uint32_t GPIO_SIZE = 0x1000;

static inline bool is_valid_addr(uint32_t addr) {
  // Check DRAM
  if (addr >= DRAM_BASE && addr < (DRAM_BASE + DRAM_SIZE)) {
    return true;
  }
  // Check UART
  if (addr >= UART_BASE && addr < (UART_BASE + UART_SIZE)) {
    return true;
  }
  // Check GPIO
  if (addr >= GPIO_BASE && addr < (GPIO_BASE + GPIO_SIZE)) {
    return true;
  }
  return false;
}

static inline uint32_t mem_read32(uint32_t addr) {
  if (addr >= DRAM_BASE && addr < (DRAM_BASE + DRAM_SIZE)) {
    // DRAM access
    uint32_t offset = addr - DRAM_BASE;
    return read_dram(offset);
  } else if (addr >= UART_BASE && addr < (UART_BASE + UART_SIZE)) {
    // UART access
    return read_uart_register(addr - UART_BASE);
  }
  // ... handle other regions
  return 0;
}
```

## Configuration Options

You can make memory layout configurable:

```cpp
// In HarnessConfig
struct MemoryConfig {
  uint32_t dram_base = 0x80000000;
  uint32_t dram_size = 64 * 1024;
  bool strict_checking = true;  // Reject unmapped access
  bool log_violations = false;  // Log invalid accesses
};

// Usage
if (config.memory.strict_checking && !is_valid_addr(addr)) {
  if (config.memory.log_violations) {
    log("Invalid access to 0x%08x", addr);
  }
  return 0; // or throw exception
}
```

## Troubleshooting

### Issue: DUT hangs after alignment

**Cause:** DUT expects memory at 0x0 but code is at 0x80000000

**Solution:** Ensure firmware/seeds use correct addresses:
```assembly
.section .text
.global _start
_start:
  # Code starts at 0x80000000
  li x1, 10
  # ...
```

### Issue: All tests fail after alignment

**Cause:** Seeds might have hardcoded low addresses

**Solution:** Regenerate seeds with correct base address:
```bash
cd firmware
make clean
make  # Rebuilds with correct linker script
```

### Issue: Spike still shows different addresses

**Cause:** Spike's bootloader runs first

**Solution:** Already handled by skipping bootloader commits:
```cpp
// In HarnessMain.cpp
if (skip_rec.pc_w >= 0x80000000) {
  skip_until = i;  // Found user code start
  break;
}
```

## Summary

✅ **Memory models now aligned:**
- Both use base address 0x80000000
- Both reject unmapped accesses
- Both have 64KB DRAM region

✅ **Benefits:**
- Eliminates false positives
- Exposes real bugs
- Realistic testing environment

✅ **Changes required:**
- Rebuild fuzzer: `make clean && make fuzz-build`
- No changes to firmware/seeds (already use correct addresses)
- Existing crashes may disappear (were false positives)

🎯 **Result:** More accurate differential testing with fewer false positives!
