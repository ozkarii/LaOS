#!/bin/sh

GDB_WRAPPER=""
BUILD_DIR="build"
QEMU_EXTRA_ARGS=""

if [ "$1" = "-d" ]; then
    GDB_WRAPPER="gdb --ex run --args"
    BUILD_DIR="build-debug"
    QEMU_EXTRA_ARGS="-s -S"
fi

${GDB_WRAPPER} qemu-system-aarch64 \
    -machine raspi4b \
    -kernel ${BUILD_DIR}/src/kernel/kernel \
    -smp 4 \
    -device loader,file=${BUILD_DIR}/src/user/init/init,addr=0x70000000,force-raw=on \
    -nographic \
    ${QEMU_EXTRA_ARGS}