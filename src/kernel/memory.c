#include <stdint.h>
#include <stdbool.h>
#include "string.h"

#include "memory.h"
#include "spinlock.h"
#include "mmu.h"

#define OBJECT_SIZE_SMALL  256
#define OBJECT_SIZE_MEDIUM 4096
#define OBJECT_SIZE_LARGE  2097152  // 2 MB

#define NUM_OBJECTS_SMALL  128
#define NUM_OBJECTS_MEDIUM 32
#define NUM_OBJECTS_LARGE  16

#define CACHE_SIZE 4

#define NUM_POOLS 3

typedef struct {
  __attribute__((aligned(OBJECT_SIZE_SMALL)))
  uint8_t data[OBJECT_SIZE_SMALL];
} ObjectPoolSmall;

typedef struct {
  __attribute__((aligned(OBJECT_SIZE_MEDIUM)))
  uint8_t data[OBJECT_SIZE_MEDIUM];
} ObjectPoolMedium;

typedef struct {
  __attribute__((aligned(OBJECT_SIZE_LARGE)))
  uint8_t data[OBJECT_SIZE_LARGE];
} ObjectPoolLarge;

static ObjectPoolSmall object_pool_small[NUM_OBJECTS_SMALL];
static ObjectPoolMedium object_pool_medium[NUM_OBJECTS_MEDIUM];
static ObjectPoolLarge object_pool_large[NUM_OBJECTS_LARGE];

static bool allocated_small[NUM_OBJECTS_SMALL] = { false };
static bool allocated_medium[NUM_OBJECTS_MEDIUM] = { false };
static bool allocated_large[NUM_OBJECTS_LARGE] = { false };

static int recently_freed_cache_small[CACHE_SIZE] = {-1, -1, -1, -1};
static int recently_freed_cache_medium[CACHE_SIZE] = {-1, -1, -1, -1};
static int recently_freed_cache_large[CACHE_SIZE] = {-1, -1, -1, -1};

Spinlock k_malloc_lock = {0};

typedef enum {
  POOL_NONE,
  POOL_SMALL,
  POOL_MED,
  POOL_LARGE
} PoolType;

typedef struct {
  void* pool_base;
  bool* allocated;
  int* cache;
  size_t object_size;
  int num_objects;
} PoolInfo;

typedef struct {
  PoolType type;
  int index;
} PointerInfo;

static PoolInfo pool_infos[NUM_POOLS] = {
  {
    .pool_base = object_pool_small,
    .allocated = allocated_small,
    .cache = recently_freed_cache_small,
    .object_size = OBJECT_SIZE_SMALL,
    .num_objects = NUM_OBJECTS_SMALL,
  },
  {
    .pool_base = object_pool_medium,
    .allocated = allocated_medium,
    .cache = recently_freed_cache_medium,
    .object_size = OBJECT_SIZE_MEDIUM,
    .num_objects = NUM_OBJECTS_MEDIUM,
  },
  {
    .pool_base = object_pool_large,
    .allocated = allocated_large,
    .cache = recently_freed_cache_large,
    .object_size = OBJECT_SIZE_LARGE,
    .num_objects = NUM_OBJECTS_LARGE,
  }
};

static PoolType get_pool_type(size_t size) {
  if (size <= OBJECT_SIZE_SMALL) {
    return POOL_SMALL;
  } else if (size <= OBJECT_SIZE_MEDIUM) {
    return POOL_MED;
  } else if (size <= OBJECT_SIZE_LARGE) {
    return POOL_LARGE;
  }
  return POOL_NONE;
}

static void* alloc_from_pool(PoolInfo* pool) {
  // Check recently freed cache first
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (pool->cache[i] != -1) {
      int idx = pool->cache[i];
      pool->allocated[idx] = true;
      pool->cache[i] = -1;
      return (uint8_t*)pool->pool_base + (idx * pool->object_size);
    }
  }

  // Find first free object
  for (int i = 0; i < pool->num_objects; i++) {
    if (!pool->allocated[i]) {
      pool->allocated[i] = true;
      return (uint8_t*)pool->pool_base + (i * pool->object_size);
    }
  }

  return NULL;  // No free objects
}

static PointerInfo get_pointer_info(void* ptr) {
  uintptr_t addr = (uintptr_t)ptr;

  for (int p = 0; p < NUM_POOLS; p++) {
    PoolInfo* pool = &pool_infos[p];
    uintptr_t pool_base = (uintptr_t)pool->pool_base;
    uintptr_t pool_size = pool->object_size * pool->num_objects;

    if (addr >= pool_base && addr < pool_base + pool_size) {
      int index = (addr - pool_base) / pool->object_size;
      if (index < pool->num_objects) {
        return (PointerInfo){p + 1, index};
      }
    }
  }

  return (PointerInfo){POOL_NONE, -1};
}

void* k_malloc(size_t size) {
  if (size < 1) {
    return NULL;
  }

  PoolType pool_type = get_pool_type(size);
  if (pool_type == POOL_NONE) {
    return NULL;  // Size too large
  }

  spinlock_acquire(&k_malloc_lock);
  void* ptr = alloc_from_pool(&pool_infos[pool_type - 1]);
  spinlock_release(&k_malloc_lock);

  return ptr;
}

void k_free(void* ptr) {
  if (ptr == NULL) {
    return;
  }

  PointerInfo info = get_pointer_info(ptr);
  if (info.type == POOL_NONE || info.index == -1) {
    return;  // Pointer not in any pool
  }

  spinlock_acquire(&k_malloc_lock);

  PoolInfo* pool = &pool_infos[info.type - 1];

  if (!pool->allocated[info.index]) {
    spinlock_release(&k_malloc_lock);
    return;  // Double free
  }

  pool->allocated[info.index] = false;

  // Add to recently freed cache
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (pool->cache[i] == -1) {
      pool->cache[i] = info.index;
      break;
    }
  }

  spinlock_release(&k_malloc_lock);
}


void* allocate_user_memory_block(uint64_t* l2_table, bool executable) {
  if (l2_table == NULL) {
    return NULL;
  }

  void* phys_base = k_malloc(BLOCK_SIZE_L2);
  if (phys_base == NULL) {
    return NULL;
  }

  // Find a free L2 entry, start from 1 since 0 is reserved
  for (int i = 1; i < L2_PAGE_TABLE_ENTRIES; i++) {
    if ((l2_table[i] & VB_MASK) == DESC_INVALID) {
      uint64_t desc = DESC_BLOCK      // Valid block descriptor
                    | AF              // Access Flag
                    | SH_INNER        // Inner Shareable
                    | INDX_NORMAL_WB  // Normal memory, Write-Back
                    | AP_RW_ALL       // RW for EL0 and EL1
                    | PXN;            // Privileged Execute Never

      if (!executable) {
        desc |= UXN;  // Unprivileged Execute Never
      }

      desc |= ((uintptr_t)phys_base & ~(BLOCK_SIZE_L2 - 1));  // Set physical base address

      l2_table[i] = desc;

      return phys_base;
    }
  }

  return NULL;  // No free L2 entries
}

void free_user_memory_block(uint64_t* l2_table, void* block_ptr) {
  if (l2_table == NULL || block_ptr == NULL) {
    return;
  }

  uintptr_t phys_addr = (uintptr_t)block_ptr;

  // Find the L2 entry corresponding to block_ptr
  for (int i = 0; i < L2_PAGE_TABLE_ENTRIES; i++) {
    uint64_t desc = l2_table[i];
    if ((desc & VB_MASK) != DESC_INVALID) {
      uintptr_t entry_addr = desc & OA_MASK;
      if (entry_addr == (phys_addr & OA_MASK)) {
        // Invalidate the L2 entry
        l2_table[i] = DESC_INVALID;
        break;
      }
    }
  }

  k_free(block_ptr);
}

/**
 * Clone a process's L2 page table (for fork-like operations).
 * This copies the L2 table structure, but both processes will
 * initially map the same physical memory (copy-on-write not implemented).
 */
void clone_process_page_table(void* dest_page_table_base, void* src_page_table_base) {
  if (dest_page_table_base != NULL && src_page_table_base != NULL) {
    memcpy(dest_page_table_base, src_page_table_base, sizeof(uint64_t) * L2_PAGE_TABLE_ENTRIES);
  }
}