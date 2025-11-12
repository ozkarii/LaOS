.section .text
.global _Reset
_Reset:
    // Set vector table base address FIRST
    ldr x0, =vector_table_el3
    msr vbar_el3, x0
    isb

    // Configure SCR_EL3 for EL3 interrupt routing
    mrs x0, scr_el3
    orr x0, x0, #(1 << 3)    // EA bit - route SErrors to EL3
    orr x0, x0, #(1 << 2)    // FIQ bit - route FIQs to EL3  
    orr x0, x0, #(1 << 1)    // IRQ bit - route IRQs to EL3
    msr scr_el3, x0
    isb

    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbnz x0, _Sleep

    ldr x1, =bss_start
    ldr x2, =bss_end
    cmp x1, x2
    beq _Set_sp

_Zero_bss_loop:
    strb wzr, [x1], #1    // Store zero and post-increment x1
    cmp x1, x2            // Compare current address with bss_end
    blt _Zero_bss_loop    // Continue if current address < bss_end


_Set_sp:
    // Set EL3 stack
    ldr x0, =stack_top_el3
    mov sp, x0

    bl c_entry
    b .

_Sleep:
    wfe
    b _Sleep
