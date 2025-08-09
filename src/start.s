.global _Reset
_Reset:
    mrs x1, mpidr_el1
    and x1, x1, #0xFF
    cbnz x1, _Infinite_loop

    ldr x30, =stack_top	     // setup stack
    sub x30, x30, #0x1000    // Start 4KB below stack_top for safety
    mov sp, x30
    
    bl c_entry
    b .

_Infinite_loop:
    b _Infinite_loop

