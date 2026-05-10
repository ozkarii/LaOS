#!/bin/bash

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

pushd "$SCRIPT_DIR" > /dev/null

./debug-run.sh &

export GDB_CMD="gf2"
./attach-gdb.sh

popd > /dev/null

pkill qemu-system-aar