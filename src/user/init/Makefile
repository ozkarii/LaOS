TOOLCHAIN_PATH := 

AS = $(TOOLCHAIN_PATH)aarch64-none-elf-as
CC = $(TOOLCHAIN_PATH)aarch64-none-elf-gcc
LD = $(TOOLCHAIN_PATH)aarch64-none-elf-ld
AR = $(TOOLCHAIN_PATH)aarch64-none-elf-ar
OBJCOPY = $(TOOLCHAIN_PATH)aarch64-none-elf-objcopy

SRCDIR := .
INCLUDE_DIRS := ../../../src/kernel/include
BUILDDIR := ../../../build/user/init
LINKER_SCRIPT := $(SRCDIR)/../link.ld

ASFLAGS = -g -march=armv8-a
CFLAGS += -g -c -march=armv8-a -std=gnu99 -nostdlib -static -nostartfiles -ffreestanding -mgeneral-regs-only -mno-outline-atomics -Wall -Wextra -I$(INCLUDE_DIRS)
LDFLAGS += -T $(LINKER_SCRIPT)

SRC_C := $(wildcard $(SRCDIR)/*.c)
SRC_S := $(wildcard $(SRCDIR)/*.s)
SRC_S += $(SRCDIR)/../crt.s

OBJ_C := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC_C))
OBJ_S := $(patsubst $(SRCDIR)/%.s,$(BUILDDIR)/%.o,$(SRC_S))
OBJ := $(OBJ_C) $(OBJ_S)

all: init

init: $(BUILDDIR)/init.bin

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.s | $(BUILDDIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/init.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/init.bin: $(BUILDDIR)/init.elf
	$(OBJCOPY) -O binary $< $@
	printf '\xDE\xAD\xBE\xEF' >> $@

clean:
	rm -rf $(BUILDDIR)
