AS = aarch64-none-elf-as
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld

ASFLAGS = -g
CFLAGS = -c -nostdlib -nostartfiles -std=gnu99 -ffreestanding -g

# Source and build directories
SRCDIR := src
BUILDDIR := build

# Find source files
SRC_C := $(wildcard $(SRCDIR)/*.c)
SRC_S := $(wildcard $(SRCDIR)/*.s)

# Generate object file paths in build directory
OBJ_C := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC_C))
OBJ_S := $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.o,$(SRC_S))
OBJ := $(OBJ_C) $(OBJ_S)

LINKER_SCRIPT := $(SRCDIR)/link.ld
LDFLAGS = -T $(LINKER_SCRIPT)

# Default target
all: $(BUILDDIR)/kernel.elf

# Create build directory if it doesn't exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build C object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@

# Build assembly object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.s | $(BUILDDIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(BUILDDIR)/kernel.elf: $(OBJ) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)

# Phony targets
.PHONY: all clean