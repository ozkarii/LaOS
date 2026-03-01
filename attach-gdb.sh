#!/bin/bash

# If GDB_CMD env var is not set, try to find gdb-multiarch or default to gdb
if [ -z $GDB_CMD ]; then	
	if command -v gdb-multiarch > /dev/null 2>&1
	then
		GDB_CMD="gdb-multiarch"
	else
		GDB_CMD="gdb"
	fi
fi

$GDB_CMD --ex "set arch aarch64" --ex "symbol build/kernel/kernel.elf" --ex "target remote localhost:1234" --ex "tui layout split" --ex "tui focus cmd" $@
