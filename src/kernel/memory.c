#include <stdlib.h>
#include "memory.h"
#include "spinlock.h"

Spinlock k_malloc_lock = {0};

void* k_malloc(size_t size) {
  spinlock_acquire(&k_malloc_lock);
  void* ptr = malloc(size);
  spinlock_release(&k_malloc_lock);
  return ptr;
}