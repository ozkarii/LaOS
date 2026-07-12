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

BUILD_DIR="build-debug"

if [ "$1" = "-b" ]; then
    BUILD_DIR="$2"
    shift 2
fi

$GDB_CMD \
    --ex "set arch aarch64" \
    --ex "symbol ${BUILD_DIR}/src/kernel/kernel" \
    --ex "add-symbol-file ${BUILD_DIR}/src/user/init/init" \
    --ex "target remote localhost:1234" \
    --ex "tui layout split" \
    --ex "tui focus cmd" \
    $@