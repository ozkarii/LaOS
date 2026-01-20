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
	stack_bottom_el3 = .;
    . = . + 0x10000;  /* 16KB stack for each CPU in EL3 */
    . = ALIGN(16);
    stack_top_el3 = .;

	. = ALIGN(16);
	stack_bottom_el1 = .;
	. = . + 0x20000;   /* 32KB stack for each CPU in EL1*/
	. = ALIGN(16);
	stack_top_el1 = .;

	. = ALIGN(16);
	kernel_heap_start = .;
	. = . + 0x4000000; /* 64MB kernel heap */
	. = ALIGN(16);
	kernel_heap_end = .;
}
