/* ============================================================
 * link.ld — robust, harness-aware linker script for RV32 firmware
 * ============================================================
 * Supports runtime-configurable addresses via -defsym or env vars:
 *   PROGADDR_RESET, STACK_ADDR, RAM_BASE, RAM_SIZE_BYTES, RAM_SIZE_ALIGNED
 *   PROGADDR_IRQ, ROM_BASE, ROM_SIZE
 * ============================================================ */

OUTPUT_ARCH(riscv)
ENTRY(_start)

/*--------------------------------------------------------------------
  Memory configuration (default → overridable by -Wl,-defsym)
--------------------------------------------------------------------*/

/* ROM and program entry base */
PROGADDR_RESET   = DEFINED(PROGADDR_RESET)   ? PROGADDR_RESET   : 0x80000000;
PROGADDR_IRQ     = DEFINED(PROGADDR_IRQ)     ? PROGADDR_IRQ     : (PROGADDR_RESET + 0x100);
ROM_BASE         = DEFINED(ROM_BASE)         ? ROM_BASE         : PROGADDR_RESET;
ROM_SIZE         = DEFINED(ROM_SIZE)         ? ROM_SIZE         : 0x00040000; /* 256 KiB */

/* RAM layout (where data/bss/stack live) */
RAM_BASE         = DEFINED(RAM_BASE)         ? RAM_BASE         : 0x80040000;
RAM_SIZE_BYTES   = DEFINED(RAM_SIZE_BYTES)   ? RAM_SIZE_BYTES   : 0x00040000; /* 256 KiB */
RAM_SIZE_ALIGNED = DEFINED(RAM_SIZE_ALIGNED) ? RAM_SIZE_ALIGNED : (RAM_SIZE_BYTES & ~0xFFF);

/* Stack top (default = end of RAM) */
STACK_ADDR       = DEFINED(STACK_ADDR)       ? STACK_ADDR       : (RAM_BASE + RAM_SIZE_ALIGNED);

/*--------------------------------------------------------------------
  Define memory regions
--------------------------------------------------------------------*/
MEMORY
{
  ROM (rx)  : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
  RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE_ALIGNED
}

/*--------------------------------------------------------------------
  Program headers (ELF PT_LOAD segments)
--------------------------------------------------------------------*/
PHDRS
{
  text PT_LOAD FLAGS(5); /* R+X */
  data PT_LOAD FLAGS(6); /* R+W */
}

/*--------------------------------------------------------------------
  Section placement
--------------------------------------------------------------------*/
SECTIONS
{
  /*------------------------------------------------------------
    .text: code + read-only data
  ------------------------------------------------------------*/
  .text : ALIGN(4)
  {
    _stext = .;
    KEEP(*(.init))
    KEEP(*(.text.boot))
    *(.text .text.*)
    *(.rodata .rodata.*)
    _etext = .;
  } >ROM :text

  /*------------------------------------------------------------
    .data: initialized variables (copied from ROM → RAM)
  ------------------------------------------------------------*/
  .data : ALIGN(4)
  {
    _sdata = .;
    *(.data .data.*)
    _edata = .;
  } >RAM AT>ROM :data

  /* ROM load address for .data */
  _ldata = LOADADDR(.data);

  /*------------------------------------------------------------
    .bss: uninitialized data (zeroed by crt0)
  ------------------------------------------------------------*/
  .bss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    _ebss = .;
  } >RAM :data

  /*------------------------------------------------------------
    Stack & heap
  ------------------------------------------------------------*/
  . = ALIGN(16);

  /* Stack grows downward from STACK_ADDR */
  __stack_top   = STACK_ADDR;
  __stack_limit = __stack_top - 0x1000;    /* 4 KiB default stack */

  /* Heap grows upward after .bss */
  __heap_start  = _ebss;
  __heap_end    = __stack_limit;
}

/*--------------------------------------------------------------------
  Provide global linker symbols
--------------------------------------------------------------------*/
PROVIDE(_stext        = _stext);
PROVIDE(_etext        = _etext);
PROVIDE(_sdata        = _sdata);
PROVIDE(_edata        = _edata);
PROVIDE(_sbss         = _sbss);
PROVIDE(_ebss         = _ebss);
PROVIDE(_ldata        = _ldata);
PROVIDE(__stack_top   = __stack_top);
PROVIDE(__stack_limit = __stack_limit);
PROVIDE(__heap_start  = __heap_start);
PROVIDE(__heap_end    = __heap_end);
PROVIDE(__irq_entry   = PROGADDR_IRQ);
