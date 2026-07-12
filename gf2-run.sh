#!/bin/bash

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

pushd "$SCRIPT_DIR" > /dev/null

./run.sh -d &

export GDB_CMD="gf2"
./attach-gdb.sh

popd > /dev/null

pkill qemu-system-aar