#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Error: This script must be sourced, not executed."
    return 1 2>/dev/null || exit 1
fi

# Use toolchain already in PATH by default  
if ! command -v aarch64-none-elf-gcc >/dev/null 2>&1; then
    # Use toolchain from toolchain/bin if not already in PATH
    if [[ ! -x "$SCRIPT_DIR/toolchain/bin/aarch64-none-elf-gcc" ]]; then
        echo "Error: aarch64-none-elf-gcc not found in $SCRIPT_DIR/toolchain/bin"
        return 1 2>/dev/null || exit 1
    fi
    export PATH="$SCRIPT_DIR/toolchain/bin:$PATH"
    echo "Added $SCRIPT_DIR/toolchain/bin to PATH"
fi

# Use QEMU from qemu-install/bin by default
if [[ -f "$SCRIPT_DIR/qemu-install/bin/qemu-system-aarch64" ]]; then
    case ":$PATH:" in
        *":$SCRIPT_DIR/qemu-install/bin:"*) ;;
        *) export PATH="$SCRIPT_DIR/qemu-install/bin:$PATH" && echo "Added $SCRIPT_DIR/qemu-install/bin to PATH";;
    esac
else
    if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
        echo "Warning: qemu-system-aarch64 not found."
    fi
fi

export LAUDES_OS_SETUP_DONE=1
echo "LAUDES_OS_SETUP_DONE=1"