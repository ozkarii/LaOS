# Toolchain path
TOOLCHAIN_PATH := 

AS = $(TOOLCHAIN_PATH)aarch64-none-elf-as
CC = $(TOOLCHAIN_PATH)aarch64-none-elf-gcc
LD = $(TOOLCHAIN_PATH)aarch64-none-elf-ld
AR = $(TOOLCHAIN_PATH)aarch64-none-elf-ar

# Source and build directories
KERNEL_SRCDIR := src/kernel
BUILDDIR := build
KERNEL_BUILDDIR := $(BUILDDIR)/kernel
LIBC_SRCDIR := src/libc
LIBC_INCLUDEDIR := src/libc/include
LIBC_BUILDDIR := $(BUILDDIR)/libc

ASFLAGS = -g -march=armv8-a
CFLAGS = -c -nostartfiles -std=gnu99 -ffreestanding -g -nostdlib -I$(LIBC_INCLUDEDIR) -Wall -Wextra -march=armv8-a -mgeneral-regs-only -mno-outline-atomics

# Find source files
SRC_C := $(wildcard $(KERNEL_SRCDIR)/*.c)
SRC_S := $(wildcard $(KERNEL_SRCDIR)/*.s)

# Find libc source files
LIBC_SRC_C := $(wildcard $(LIBC_SRCDIR)/*.c)
LIBC_SRC_S := $(wildcard $(LIBC_SRCDIR)/*.s)

# Generate object file paths in build directory
OBJ_C := $(patsubst $(KERNEL_SRCDIR)/%.c,$(KERNEL_BUILDDIR)/%.o,$(SRC_C))
OBJ_S := $(patsubst $(KERNEL_SRCDIR)/%.s,$(KERNEL_BUILDDIR)/%.o,$(SRC_S))
OBJ := $(OBJ_C) $(OBJ_S)

# Generate libc object file paths
LIBC_OBJ_C := $(patsubst $(LIBC_SRCDIR)/%.c,$(LIBC_BUILDDIR)/%.o,$(LIBC_SRC_C))
LIBC_OBJ_S := $(patsubst $(LIBC_SRCDIR)/%.s,$(LIBC_BUILDDIR)/%.o,$(LIBC_SRC_S))
LIBC_OBJ := $(LIBC_OBJ_C) $(LIBC_OBJ_S)

LINKER_SCRIPT := $(KERNEL_SRCDIR)/link.ld
LDFLAGS = -T $(LINKER_SCRIPT)
LIBC_CFLAGS = $(CFLAGS) -I$(LIBC_INCLUDEDIR)

# Default target
all: $(KERNEL_BUILDDIR)/kernel.elf

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


# Build C object files
$(KERNEL_BUILDDIR)/%.o: $(KERNEL_SRCDIR)/%.c | $(KERNEL_BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@

# Build assembly object files
$(KERNEL_BUILDDIR)/%.o: $(KERNEL_SRCDIR)/%.s | $(KERNEL_BUILDDIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_BUILDDIR)/kernel.elf: $(OBJ) $(LIBC_BUILDDIR)/libc.a $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBC_BUILDDIR)/libc.a

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)

# Phony targets
.PHONY: all clean libc