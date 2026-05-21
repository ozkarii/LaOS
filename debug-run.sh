#!/bin/bash

gdb --ex "run" --args qemu-system-aarch64 -machine raspi4b -kernel build/src/kernel/kernel -smp 4 -device loader,file=build/src/user/init/init,addr=0x70000000,force-raw=on -nographic -s -S $@
