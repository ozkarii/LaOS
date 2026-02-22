#!/bin/bash

gdb --ex "run" --args qemu-system-aarch64 -machine raspi4b -kernel build/kernel/kernel.elf -smp 4 -device loader,file=build/user/init/init.bin,addr=0x70000000,force-raw=on -nographic -s -S $@
