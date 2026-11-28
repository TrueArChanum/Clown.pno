
/* Part of Penetrizzel         */
/* Copyright (c) 2026 ArChanum */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(pno_entry)

PHDRS
{
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 0;
    __pno_image_start = .;
    .text : ALIGN(16)
    {
        __pno_text_start = .;
        KEEP(*(.text.pno_entry))
        *(.text)
        *(.text.*)
        . = ALIGN(16);
        __pno_text_end = .;
    } :text
    .rodata : ALIGN(16)
    {
        __pno_rodata_start = .;
        BYTE(0)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(16);
        __pno_rodata_end = .;
    } :rodata
    .data : ALIGN(16)
    {
        __pno_data_start = .;
        BYTE(0)
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        . = ALIGN(16);
        __pno_data_end = .;
    } :data
    .bss (NOLOAD) : ALIGN(16)
    {
        __pno_bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        . = ALIGN(16);
        __pno_bss_end = .;
    } :data
    __pno_bss_rva = __pno_bss_start;
    __pno_bss_size = __pno_bss_end - __pno_bss_start;
    __pno_image_end = .;
    __pno_image_size = __pno_image_end - __pno_image_start;
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.eh_frame_hdr)
        *(.got*)
        *(.plt*)
        *(.iplt*)
    }
    ASSERT(__pno_image_end < 0x80000000, "PNO image exceeds supported RVA range")
}
