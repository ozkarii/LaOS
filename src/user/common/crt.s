.section .text
.global _start
.extern stack_top

_start:
    bl main
    // Make exit syscall with main return value
    // svc x0...
