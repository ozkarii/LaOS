.section .text
.global _start
.extern stack_top

_start:
    ldr x0, =stack_top
    mov sp, x0
    bl main
    // Make exit syscall with main return value
    // svc x0...
