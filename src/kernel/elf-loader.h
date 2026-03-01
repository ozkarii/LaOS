
#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include "vfs.h"

// Loads ELF from memory, returns 0 on success, -1 on failure
int elf_load_from_memory(const void* elf_data, size_t elf_size,
                         void* exec_block, void* non_exec_block,
                         uint64_t* entry_offset);

#endif // ELF_LOADER_H
