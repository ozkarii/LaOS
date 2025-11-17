gdb --ex "set arch aarch64" --ex "file build/kernel.elf" --ex "b c_entry" --ex "target remote :1234"
