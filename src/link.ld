ENTRY(_Reset)
SECTIONS
{
	. = 0x40000000;
	
	/* Vector table section - must be aligned to 2KB */
	.vectors . : ALIGN(0x800) { 
		*(.vectors)
		. = ALIGN(0x800);
	}
	
	.startup . : { *start.o(.text) }
	.text : { *(.text) }
	
	/* Read-only data */
	.rodata : { *(.rodata*) }
	
	.data : { *(.data) }
	.bss : 
	{
		bss_start = .;
		*(.bss COMMON)
		bss_end = .;
	}
    
    /* EL3 Stack */
    . = ALIGN(16);
    . = . + 0x8000;    /* 32KB stack for EL3 */
    stack_top_el3 = .;
    
    /* EL1 Stack */
    . = ALIGN(16);
    . = . + 0x8000;    /* 32KB stack for EL1 */
    stack_top_el1 = .;
}
