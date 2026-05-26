ENTRY(_start)

SECTIONS
{
    /* Executable */
    . = 0x200000;

    .text : {
        *(.text .text.*)
    }

    /* Data */
    . = 0x400000;

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
}