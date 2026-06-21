.section .text
.global _start
.extern stack_top

_start:
    bl main
    bl _exit
