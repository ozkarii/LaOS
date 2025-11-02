#!/bin/sh
qemu-system-aarch64 -machine raspi4b -kernel build/kernel.elf -smp 4 -nographic $@
