ENTRY(_start)

SECTIONS
{
    /* Executable */
    . = 0x1000; /* Start from offset of 4 KB */

    .text : {
        *(.text .text.*)
    }

    /* Non-executable */
    /* Next 2 MB block */
    /*. = 0x200000;*/

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

    . = ALIGN(16);
    stack_bottom = .;
    . = . + 0x10000; /* 64 KB stack */
    stack_top = .;
}