# Recreational OS Project
Intended for teaching myself OS implementation and 64-bit ARM architecture.

## Features
- Preemptive scheduling of kernel and user tasks
- Virtual filesystem
- RAM filsystem implementation
- Isolated user processes (WIP)
- SMP support
- Semaphores and spinlocks in the kernel
- Virtual memory with two-level page table hierarchy implemented
- Interrupts with GICv2
- Minimal C library for kernel use
- Simple object pool allocator for paging and other kernel objects
- pl011 UART Console

## Limitations
- Targets only aarch64 raspi4b
- Only tested in QEMU
- Not designed to be very portable
- Memory layout is quite static