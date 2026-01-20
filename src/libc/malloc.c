#include <stdint.h>
#include "malloc.h"
#include "stdlib.h"
#include "string.h"

#define KERNEL_HEAP_SIZE ((uint64_t)((uintptr_t)kernel_heap_end - (uintptr_t)kernel_heap_start))

extern uint64_t kernel_heap_start[];
extern uint64_t kernel_heap_end[];

// Simple bump allocator for kernel heap
void* malloc(size_t size) {
    static uintptr_t current_heap_ptr = (uintptr_t)kernel_heap_start;
    if ((uint64_t)(current_heap_ptr + size) >
        (uint64_t)(kernel_heap_start + KERNEL_HEAP_SIZE))
    {
        return NULL; // Out of memory
    }
    uintptr_t aligned_size = (size + 0xF) & ~0xFUL; // Align to 16 bytes
    uintptr_t allocated_ptr = current_heap_ptr;
    current_heap_ptr += aligned_size;
    return (void*)allocated_ptr;
}
