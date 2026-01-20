# Use toolchain from path from environment variable or default to system path
TOOLCHAIN_PATH := 

AS = $(TOOLCHAIN_PATH)aarch64-none-elf-as
CC = $(TOOLCHAIN_PATH)aarch64-none-elf-gcc
LD = $(TOOLCHAIN_PATH)aarch64-none-elf-ld
AR = $(TOOLCHAIN_PATH)aarch64-none-elf-ar

# Source, build and exported include directories
BUILDDIR := build

KERNEL_SRCDIR := src/kernel
KERNEL_INCLUDEDIR := src/kernel/include
KERNEL_BUILDDIR := $(BUILDDIR)/kernel

LIBC_SRCDIR := src/libc
LIBC_INCLUDEDIR := src/libc/include
LIBC_BUILDDIR := $(BUILDDIR)/libc

USER_SRCDIR := src/user
USER_BUILDDIR := $(BUILDDIR)/user

# Common assembler and compiler flags
ASFLAGS = -g -march=armv8-a
CFLAGS = -c -nostartfiles -std=gnu99 -ffreestanding -g -nostdlib -Wall -Wextra -march=armv8-a -mgeneral-regs-only -mno-outline-atomics

# Find kernel source files
KERNEL_SRC_C := $(wildcard $(KERNEL_SRCDIR)/*.c)
KERNEL_SRC_S := $(wildcard $(KERNEL_SRCDIR)/*.s)

# Find libc source files
LIBC_SRC_C := $(wildcard $(LIBC_SRCDIR)/*.c)
LIBC_SRC_S := $(wildcard $(LIBC_SRCDIR)/*.s)

# Generate object file paths in build directory
KERNEL_OBJ_C := $(patsubst $(KERNEL_SRCDIR)/%.c,$(KERNEL_BUILDDIR)/%.o,$(KERNEL_SRC_C))
KERNEL_OBJ_S := $(patsubst $(KERNEL_SRCDIR)/%.s,$(KERNEL_BUILDDIR)/%.o,$(KERNEL_SRC_S))
KERNEL_OBJ := $(KERNEL_OBJ_C) $(KERNEL_OBJ_S)

# Generate libc object file paths
LIBC_OBJ_C := $(patsubst $(LIBC_SRCDIR)/%.c,$(LIBC_BUILDDIR)/%.o,$(LIBC_SRC_C))
LIBC_OBJ_S := $(patsubst $(LIBC_SRCDIR)/%.s,$(LIBC_BUILDDIR)/%.o,$(LIBC_SRC_S))
LIBC_OBJ := $(LIBC_OBJ_C) $(LIBC_OBJ_S)

KERNEL_CFLAGS = $(CFLAGS) -I$(KERNEL_INCLUDEDIR) -I$(LIBC_INCLUDEDIR)
KERNEL_LINKER_SCRIPT := $(KERNEL_SRCDIR)/link.ld
KERNEL_LDFLAGS = -T $(KERNEL_LINKER_SCRIPT)

LIBC_CFLAGS = $(CFLAGS) -I$(LIBC_INCLUDEDIR)

# All target
all: kernel libc

# Kernel target
kernel: $(KERNEL_BUILDDIR)/kernel.elf

# libc target
libc: $(LIBC_BUILDDIR)/libc.a

# Create build directories if they don't exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(KERNEL_BUILDDIR):
	mkdir -p $(KERNEL_BUILDDIR)

$(LIBC_BUILDDIR):
	mkdir -p $(LIBC_BUILDDIR)

# Build libc C object files
$(LIBC_BUILDDIR)/%.o: $(LIBC_SRCDIR)/%.c | $(LIBC_BUILDDIR)
	$(CC) $(LIBC_CFLAGS) $< -o $@

# Build libc assembly object files
$(LIBC_BUILDDIR)/%.o: $(LIBC_SRCDIR)/%.s | $(LIBC_BUILDDIR)
	$(AS) $(ASFLAGS) $< -o $@

# Build libc static library
$(LIBC_BUILDDIR)/libc.a: $(LIBC_OBJ)
	$(AR) rcs $@ $^

# Build kernel C object files
$(KERNEL_BUILDDIR)/%.o: $(KERNEL_SRCDIR)/%.c | $(KERNEL_BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@

# Build kernel assembly object files
$(KERNEL_BUILDDIR)/%.o: $(KERNEL_SRCDIR)/%.s | $(KERNEL_BUILDDIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_BUILDDIR)/kernel.elf: $(KERNEL_OBJ) $(LIBC_BUILDDIR)/libc.a $(KERNEL_LINKER_SCRIPT)
	$(LD) $(KERNEL_LDFLAGS) -o $@ $(KERNEL_OBJ) $(LIBC_BUILDDIR)/libc.a

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)

# Phony targets
.PHONY: all clean libc