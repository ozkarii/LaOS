.global _Reset
_Reset:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbnz x0, _Infinite_loop

    ldr x1, =bss_start
    ldr x2, =bss_end
    cmp x1, x2
    beq _Set_sp

    mov x3, #0  // offset
_Zero_bss_loop:
    strb wzr, [x1], #1    // Store zero and post-increment x1
    cmp x1, x2            // Compare current address with bss_end
    blt _Zero_bss_loop    // Continue if current address < bss_end


_Set_sp:
    ldr x30, =stack_top  // setup stack
    mov sp, x30
    
    bl c_entry
    b .

_Infinite_loop:
    b _Infinite_loop

