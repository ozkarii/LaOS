ENTRY(_Reset)
SECTIONS
{
	. = 0x40000000;  /* 1 GiB */
	kernel_base = .;
	
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

	. = ALIGN(16);
	.stack_bottom = .;
    . = . + 0x10000;  /* 16KB of stack for each of the CPUs in EL3*/
    . = ALIGN(16);
    stack_top = .;

	. = ALIGN(16);
	.stack_bottom_el1 = .;
	. = . + 0x8000;   /* 32KB stack for EL1 CPU0 */
	. = ALIGN(16);
	stack_top_el1 = .;
}
