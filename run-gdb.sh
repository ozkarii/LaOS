#!/bin/sh
gdb --args qemu-system-aarch64 -machine raspi4b -kernel build/kernel.elf -smp 4 -nographic $@ -s -S
