ENTRY(_Reset)
SECTIONS
{
	. = 0x40000000;
	.startup . : { *start.o(.text) }
	.text : { *(.text) }
	.data : { *(.data) }
	.bss : { *(.bss COMMON) }
	. = ALIGN(16);
	. = . + 0x10000;
	stack_top = .;
}
