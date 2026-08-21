#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// TODO: move allocators somewhere else

// Kernel malloc with object pool allocation and spinlock protection
void* k_malloc(size_t size);
void* k_zalloc(size_t size);
void k_free(void* ptr);

// IRQ safe version for small allocations
// k_free can be called from IRQ context as well
// but freeing a non-IRQ-safe allocation from IRQ context is unsafe
void* k_malloc_irq_safe(size_t size);


typedef struct VirtualMemoryMapping {
  void* va;
  void* pa;
  size_t size;
  bool executable;
  uint64_t* l2_entry;
} VirtualMemoryMapping;

// Allocate a 2 MB user memory block and map it into the provided L2 page table.
// The mapping is returned in out_mapping. Returns 0 on success, -1 on failure
int allocate_user_memory_block(uint64_t* l2_table, bool executable,
                               VirtualMemoryMapping* out_mapping);

// Free a previously allocated user memory block and invalidate its L2 page table entry
void free_user_memory_block(VirtualMemoryMapping* mapping);


#endif // MEMORY_H