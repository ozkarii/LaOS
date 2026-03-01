
#include "elf-loader.h"
#include "memory.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define ELF_MAGIC 0x464C457F
#define PT_LOAD   1

typedef struct {
	uint32_t magic;
	uint8_t  elf_class;
	uint8_t  data;
	uint8_t  version;
	uint8_t  os_abi;
	uint8_t  abi_version;
	uint8_t  pad[7];
	uint16_t type;
	uint16_t machine;
	uint32_t version2;
	uint64_t entry;
	uint64_t phoff;
	uint64_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} Elf64_Ehdr;

typedef struct {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
} Elf64_Phdr;

int elf_load_from_memory(const void* elf_data, size_t elf_size,
                         void* exec_block, void* non_exec_block,
                         uint64_t* entry_offset) {
	if (!elf_data || elf_size < sizeof(Elf64_Ehdr)) {
		return -1;
	}

	const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)elf_data;
	if (ehdr->magic != ELF_MAGIC || ehdr->phnum == 0) {
		return -1;
	}

	const uint8_t* base = (const uint8_t*)elf_data;

	*entry_offset = ehdr->entry;

	for (int i = 0; i < ehdr->phnum; i++) {
		const Elf64_Phdr* phdr = (const Elf64_Phdr*)(base + ehdr->phoff + i * ehdr->phentsize);
		if (phdr->type != PT_LOAD) {
      		continue;
    	}

		bool executable = (phdr->flags & 0x1) != 0; // PF_X
		void* dest = executable ? exec_block : non_exec_block;
		if (!dest) {
      		return -1;
    	}

		if (phdr->filesz > 0 && phdr->offset + phdr->filesz <= elf_size) {
			memcpy(dest, base + phdr->offset, phdr->filesz);
			// Zero out the rest if memsz > filesz
			if (phdr->memsz > phdr->filesz) {
				memset((uint8_t*)dest + phdr->filesz, 0, phdr->memsz - phdr->filesz);
			}
		}
	}

	return 0;
}
