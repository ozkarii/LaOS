AS = aarch64-none-elf-as
CC = aarch64-none-elf-gcc
LD = aarch64-none-elf-ld

ASFLAGS = -g
CFLAGS = -c -nostdlib -nostartfiles -std=gnu99 -ffreestanding -g

SRC_C := $(wildcard *.c)
SRC_S := $(wildcard *.s)
OBJ := $(SRC_C:.c=.o) $(SRC_S:.s=.o)
LINKER_SCRIPT := link.ld
LDFLAGS = -T $(LINKER_SCRIPT)

all: kernel

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

kernel: $(OBJ) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

clean:
	rm -f *.o kernel

.PHONY: