#include "string.h"
#include "mmu.h"
#include "spinlock.h"
#include "memory.h"

// Needs 4KB alignment
__attribute__((aligned(4096)))
static uint64_t l1_page_table[L1_PAGE_TABLE_ENTRIES];

// Kernel base address from linker script
extern uint64_t kernel_base[];
extern uint64_t device_memory_base[];

static Spinlock mmu_lock = {0};

void mmu_init(bool primary_core) {

  if (primary_core) {
    // Userspace
    l1_page_table[0] = DESC_INVALID;
    
    // Kernel
    const uint64_t kernel_block_base = (uint64_t)kernel_base & ~(BLOCK_SIZE_L1 - 1);
    l1_page_table[1] = UXN | kernel_block_base | AF | SH_INNER | INDX_NORMAL_WB | AP_RW_EL1 | DESC_BLOCK;
    
    // Unused (also seems to be unaccessable in QEMU)
    l1_page_table[2] = DESC_INVALID;
    
    // MMIO
    const uint64_t mmio_block_base = (uint64_t)device_memory_base & ~(BLOCK_SIZE_L1 - 1);
    l1_page_table[3] = PXN | UXN | mmio_block_base | AF | INDX_DEVICE | AP_RW_EL1 | DESC_BLOCK;
  }

  // Configure MAIR_EL1 with memory attribute attributes
  uint64_t mair = (MAIR_DEVICE_nGnRnE << 0)     // Index 0
                | (MAIR_NORMAL_WB << 8)         // Index 1
                | (MAIR_NORMAL_NC << 16);       // Index 2
  __asm__ __volatile__ ("msr mair_el1, %0" :: "r"(mair));

  // We have VAs until 0xFFFFFFFF = 32-bit address space
  __asm__ __volatile__ ("msr tcr_el1, %0" :: "r"(32UL));

  // Load L1 page table
  __asm__ __volatile__ ("msr ttbr0_el1, %0" :: "r"((uint64_t)l1_page_table));

  // Barriers
  __asm__ __volatile__ ("dsb sy; isb");

  // Enable MMU and caches
  uint64_t sctlr;
  __asm__ __volatile__ ("mrs %0, sctlr_el1" : "=r"(sctlr));
  sctlr |= (1 << 0) | (1 << 2) | (1 << 12);
  __asm__ __volatile__ ("msr sctlr_el1, %0" :: "r"(sctlr));
  __asm__ __volatile__ ("dsb sy; isb");
}

uint64_t* mmu_create_user_l2_table(void) {
  void* l2_table = k_malloc(L2_PAGE_TABLE_ENTRIES * sizeof(uint64_t));
  if (l2_table == NULL) {
    return NULL;
  }
  memset(l2_table, 0, sizeof(uint64_t) * L2_PAGE_TABLE_ENTRIES);

  // First entry shall be invalid
  ((uint64_t*)l2_table)[0] = DESC_INVALID;

  return (uint64_t*)l2_table;
}

void mmu_free_user_l2_table(uint64_t* l2_table) {
  if (l2_table != NULL) {
    k_free(l2_table);
  }
}

void mmu_set_user_l2_table(uint64_t* l2_table) {
  spinlock_acquire(&mmu_lock);
  
  if (l2_table == NULL) {
    // Invalidate user mappings
    l1_page_table[0] = DESC_INVALID;
  } else {
    // Create L1 table descriptor pointing to L2 table
    uint64_t desc = DESC_TABLE | ((uint64_t)l2_table & OA_MASK);
    l1_page_table[0] = desc;
  }
  
  // TLB invalidation after page table modification
  __asm__ __volatile__ (
    "dsb ishst\n"           // Ensure page table write is visible
    "tlbi vmalle1is\n"      // Invalidate all TLB entries (inner shareable)
    "dsb ish\n"             // Wait for invalidation to complete
    "isb\n"                 // Synchronize
    ::: "memory"
  );
  
  spinlock_release(&mmu_lock);
}
