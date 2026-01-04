.section .vectors
.global vector_table_el3
.balign 0x800
vector_table_el3:
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


// Exception handler wrappers - save context and call C handlers
.section .text
handle_sync_exception_sp0:
handle_sync_exception_spx:
handle_sync_exception_lower_64:
handle_sync_exception_lower_32:
    // Save minimal context
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    
    bl sync_exception_handler
    
    // Restore context
    ldp x29, x30, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

handle_irq_exception_sp0:
handle_irq_exception_spx:
handle_irq_exception_lower_64:
handle_irq_exception_lower_32:
    // Save minimal context
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    
    bl irq_exception_handler
    
    // Restore context
    ldp x29, x30, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

handle_fiq_exception_sp0:
handle_fiq_exception_spx:
handle_fiq_exception_lower_64:
handle_fiq_exception_lower_32:
    // Save minimal context
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    
    bl fiq_exception_handler
    
    // Restore context
    ldp x29, x30, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret

handle_serror_exception_sp0:
handle_serror_exception_spx:
handle_serror_exception_lower_64:
handle_serror_exception_lower_32:
    // Save minimal context
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    
    bl serror_exception_handler
    
    // Restore context
    ldp x29, x30, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
    eret
