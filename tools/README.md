# Memory Configuration System

This directory contains the runtime-configurable memory layout system that ensures consistency between firmware, Verilator DUT, and AFL harness.

## Files

- **`memory_config.mk`** - Central memory layout configuration
- **`link.ld`** - Runtime-configurable linker script for firmware
- **`verilator_config_example.mk`** - Example Verilator integration

## How It Works

### 1. Central Configuration (`memory_config.mk`)

All memory addresses are defined in one place:

```makefile
PROGADDR_RESET   ?= 0x80000000  # Program entry point
PROGADDR_IRQ     ?= 0x80000100  # Interrupt vector
ROM_BASE         ?= 0x80000000  # ROM start address
ROM_SIZE         ?= 0x00040000  # 256 KiB ROM
RAM_BASE         ?= 0x80040000  # RAM start address
RAM_SIZE_BYTES   ?= 0x00040000  # 256 KiB RAM
STACK_ADDR       ?= 0x8007FFF0  # Stack top
```

### 2. Firmware Integration

The firmware Makefile includes `memory_config.mk` and passes values to the linker:

```makefile
include ../tools/memory_config.mk

LDFLAGS := -T../tools/link.ld \
           -defsym PROGADDR_RESET=$(PROGADDR_RESET) \
           -defsym RAM_BASE=$(RAM_BASE) \
           -defsym STACK_ADDR=$(STACK_ADDR) \
           ...
```

The linker script (`link.ld`) uses these values:

```ld
PROGADDR_RESET = DEFINED(PROGADDR_RESET) ? PROGADDR_RESET : 0x80000000;
RAM_BASE       = DEFINED(RAM_BASE)       ? RAM_BASE       : 0x80040000;
STACK_ADDR     = DEFINED(STACK_ADDR)     ? STACK_ADDR     : 0x8007FFF0;
```

### 3. Verilator/Harness Integration

Your Verilator build or AFL harness Makefile includes `memory_config.mk` and passes values as C defines:

```makefile
include ../tools/memory_config.mk

CFLAGS := -DPROGADDR_RESET=$(PROGADDR_RESET) \
          -DRAM_BASE=$(RAM_BASE) \
          -DSTACK_ADDR=$(STACK_ADDR)
```

In your C++ code:

```cpp
#ifndef PROGADDR_RESET
#define PROGADDR_RESET 0x80000000
#endif

void init_memory() {
    ram_base = RAM_BASE;
    ram_size = RAM_SIZE_BYTES;
    stack_top = STACK_ADDR;
}
```

## Usage

### Build Firmware with Default Addresses

```bash
cd firmware
make
```

### Build Firmware with Custom Addresses

```bash
cd firmware
make PROGADDR_RESET=0x10000000 STACK_ADDR=0x1000FFF0
```

### Build Harness with Matching Addresses

```bash
cd afl_harness
make PROGADDR_RESET=0x10000000 STACK_ADDR=0x1000FFF0
```

### Override at Runtime

Set environment variables before building:

```bash
export PROGADDR_RESET=0x10000000
export RAM_BASE=0x20000000
export STACK_ADDR=0x2007FFF0

make -C firmware
make -C afl_harness
```

## Linker Script Symbols

The linker script provides these symbols for use in firmware:

- `_stext`, `_etext` - Text section bounds
- `_sdata`, `_edata` - Data section bounds
- `_sbss`, `_ebss` - BSS section bounds
- `_ldata` - ROM address of data section (for copying)
- `__stack_top` - Top of stack
- `__stack_limit` - Stack overflow guard
- `__heap_start`, `__heap_end` - Heap region
- `__irq_entry` - Interrupt entry point

## Benefits

✅ **Single source of truth** - All addresses in one file  
✅ **Build-time validation** - Mismatches caught early  
✅ **Easy experimentation** - Change addresses without editing multiple files  
✅ **Harness consistency** - Firmware and DUT always match  
✅ **Runtime flexibility** - Override via command line or environment
