/* tools/link.ld — clean and portable for RV32 bare-metal firmware */
ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);  /* R + X */
  data PT_LOAD FLAGS(6);  /* R + W */
}

SECTIONS
{
  /* place code at 0x80000000 */
  . = 0x80000000;

  .text : ALIGN(4)
  {
    KEEP(*(.text))
    KEEP(*(.text.*))
    KEEP(*(.rodata))
    KEEP(*(.rodata.*))
  } :text

  /* place data after text */
  . = ALIGN(0x1000);
  .data : ALIGN(4)
  {
    *(.data)
    *(.data.*)
  } :data

  /* place bss after data */
  . = ALIGN(0x1000);
  .bss : ALIGN(4)
  {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  } :data
}
