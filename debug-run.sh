#!/bin/bash

gdb --ex "run" --args qemu-system-aarch64 -machine raspi4b -kernel build/kernel/kernel.elf -smp 4 -nographic -s -S $@
