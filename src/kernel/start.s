.section .text
.global _Reset
_Reset:
    // Set vector tables first
    ldr x1, =vector_table_el3
    msr vbar_el3, x1
    
    ldr x1, =vector_table_el2
    msr vbar_el2, x1

    ldr x1, =vector_table_el1
    msr vbar_el1, x1

    // Uncomment to configure for EL3 interrupt routing
    // mrs x0, scr_el3
    // orr x0, x0, #(1<<3) // The EA bit.
    // orr x0, x0, #(1<<1) // The IRQ bit.
    // orr x0, x0, #(1<<2) // The FIQ bit.
    // msr scr_el3, x0

_Init_sp:
    // Initialize the stack pointer for each core in EL3
    ldr x1, =stack_top_el3
    mrs x2, mpidr_el1
    and x2, x2, #0xFF      // x2 == CPU number.
    mov x3, #0x4000        // 16KB stack per core
    mul x3, x2, x3         // Create separated stack spaces
    sub x1, x1, x3         // for each processor
    mov sp, x1

_Setup_EL2:
    // Initialize SCTLR_EL2 and HCR_EL2 to save values before entering EL2.
    msr sctlr_el2, xzr
    msr hcr_el2, xzr

    // Determine the EL2 Execution state.
    mrs x0, scr_el3
    orr x0, x0, #(1<<10)    // RW EL2 Execution state is AArch64.
    orr x0, x0, #(1<<0)     // NS EL1 is Non-secure world.
    msr scr_el3, x0
    mov x0, #0b01001        // DAIF=0000
    msr spsr_el3, x0        // M[4:0]=01001 EL2h must match SCR_EL3.RW

    // Determine EL2 entry.
    adr x0, _EL2_entry      // el2_entry points to the first instruction of
    msr elr_el3, x0         // EL2 code.

    eret                    // Transition to EL2

_EL2_entry:
_Setup_EL1:
    // Initialize the SCTLR_EL1 register before entering EL1.
    msr sctlr_el1, xzr
    mrs x0, hcr_el2
    orr x0, x0,             #(1<<31) // RW=1 EL1 Execution state is AArch64.
    msr hcr_el2, x0
    mov x0, #0b00101        // DAIF=0000
    msr spsr_el2, x0        // M[4:0]=00101 EL1h must match HCR_EL2.RW.
    adr x0, _EL1_entry      // el1_entry points to the first instruction of
    msr elr_el2, x0         // EL1 code.

    // Setup SP_EL1
    ldr x0, =stack_top_el1
    mrs x2, mpidr_el1
    and x2, x2, #0xFF      // x2 == CPU number.
    mov x3, #0x8000        // 32KB stack per core
    mul x3, x2, x3         // Create separated stack spaces
    sub x0, x0, x3         // for each processor
    msr sp_el1, x0

    eret                    // Transition to EL1

_EL1_entry:
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    cbnz x0, _C_entry_secondary_core // If not CPU0, go to secondary core entry
_Zero_bss:
    ldr x1, =bss_start
    ldr x2, =bss_end
    cmp x1, x2
    beq _C_entry          // Skip zeroing if bss section is empty

_Zero_bss_loop:
    strb wzr, [x1], #1    // Store zero and post-increment x1
    cmp x1, x2            // Compare current address with bss_end
    blt _Zero_bss_loop    // Continue if current address < bss_end

_C_entry:
    bl c_entry
    b .

_C_entry_secondary_core:
    bl c_entry_secondary_core
    b .
