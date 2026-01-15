# Recreational OS Project
Intended for teaching myself OS implementation and 64-bit ARM architecture.

## Features
- Preemptive scheduling of tasks
- SMP support
- Semaphores and spinlocks
- Virtual memory (just identity mapping for now)
- Interrupts with GICv2
- Minimal C library for kernel use
- pl011 UART Console

## Limitations
- Targets only aarch64 raspi4b
- Only tested in QEMU
- Not designed to be very portable

## Work in progress
Implementation of userspace is ongoing.
