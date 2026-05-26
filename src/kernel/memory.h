#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>


// Kernel malloc with object pool allocation and spinlock protection
void* k_malloc(size_t size);
void k_free(void* ptr);

typedef struct VirtualMemoryMapping {
  void* va;
  void* pa;
  size_t size;
  bool executable;
  uint64_t* l2_entry;
} VirtualMemoryMapping;

// Clone page table from src to dest, both must be allocated already
void clone_process_page_table(void* dest_page_table_base, void* src_page_table_base);

// Allocate a 2 MB user memory block in given L2 page table
// Returns the physical address of the allocated block, or NULL on failure
int allocate_user_memory_block(uint64_t* l2_table, bool executable,
                               VirtualMemoryMapping* out_mapping);

// Free a previously allocated user memory block and invalidate its L2 page table entry
void free_user_memory_block(uint64_t* l2_table, VirtualMemoryMapping* mapping);


#endif // MEMORY_H