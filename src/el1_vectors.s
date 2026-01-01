.section .vectors
.global vector_table_el1
.balign 0x800
vector_table_el1:
    // Current EL with SP0
    .balign 0x80
    b handle_sync_exception_sp0       // Synchronous
    .balign 0x80
    b handle_irq_exception_sp0        // IRQ
    .balign 0x80
    b handle_fiq_exception_sp0        // FIQ
    .balign 0x80
    b handle_serror_exception_sp0     // SError

    // Current EL with SPx
    .balign 0x80
    b handle_sync_exception_spx       // Synchronous
    .balign 0x80
    b handle_irq_exception_spx        // IRQ
    .balign 0x80
    b handle_fiq_exception_spx        // FIQ
    .balign 0x80
    b handle_serror_exception_spx     // SError

    // Lower EL using AArch64
    .balign 0x80
    b handle_sync_exception_lower_64  // Synchronous
    .balign 0x80
    b handle_irq_exception_lower_64   // IRQ
    .balign 0x80
    b handle_fiq_exception_lower_64   // FIQ
    .balign 0x80
    b handle_serror_exception_lower_64 // SError

    // Lower EL using AArch32
    .balign 0x80
    b handle_sync_exception_lower_32  // Synchronous
    .balign 0x80
    b handle_irq_exception_lower_32   // IRQ
    .balign 0x80
    b handle_fiq_exception_lower_32   // FIQ
    .balign 0x80
    b handle_serror_exception_lower_32 // SError


// Push context macro
.macro PUSH_CONTEXT
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    str x30, [sp, #-8]!
.endm

// Pop context macro
.macro POP_CONTEXT
    ldr x30, [sp], #8
    ldp x28, x29, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
.endm


// Exception handler wrappers - save context and call C handlers
.section .text
handle_sync_exception_sp0:
handle_sync_exception_spx:
handle_sync_exception_lower_64:
handle_sync_exception_lower_32:
    PUSH_CONTEXT
    bl sync_exception_handler
    POP_CONTEXT
    eret

handle_irq_exception_sp0:
handle_irq_exception_spx:
handle_irq_exception_lower_64:
handle_irq_exception_lower_32:
    PUSH_CONTEXT
    mov x0, sp
    bl irq_exception_handler
    POP_CONTEXT
    eret

handle_fiq_exception_sp0:
handle_fiq_exception_spx:
handle_fiq_exception_lower_64:
handle_fiq_exception_lower_32:
    PUSH_CONTEXT
    bl fiq_exception_handler
    POP_CONTEXT
    eret

handle_serror_exception_sp0:
handle_serror_exception_spx:
handle_serror_exception_lower_64:
handle_serror_exception_lower_32:
    PUSH_CONTEXT
    bl serror_exception_handler
    POP_CONTEXT
    eret
