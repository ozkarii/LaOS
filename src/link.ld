ENTRY(_Reset)
SECTIONS
{
	. = 0x40000000;
	.startup . : { *start.o(.text) }
	.text : { *(.text) }
	.data : { *(.data) }
	.bss : 
	{
		bss_start = .;
		*(.bss COMMON)
		bss_end = .;
	}
    
	/* Heap section */
    . = ALIGN(16);
    heap_start = .;
    . = . + 0x4000000;  /* 64MB heap */
    heap_end = .;
    
    /* Stack section */
    . = ALIGN(16);
    . = . + 0x10000;   /* 64KB stack */
    stack_top = .;
}
