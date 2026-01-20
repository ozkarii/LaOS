#include "mmu.h"

__attribute__((aligned(PAGE_SIZE)))
static uint64_t l1_page_table[L1_PAGE_TABLE_ENTRIES];

// Kernel base address from linker script
extern uint64_t kernel_base[];

void mmu_init(bool primary_core) {
  
  if (primary_core) {
    // Unused
    l1_page_table[0] = DESC_INVALID;
    
    // Kernel
    const uint64_t kernel_block_base = (uint64_t)kernel_base & ~(BLOCK_SIZE - 1);
    l1_page_table[1] = UXN | kernel_block_base | AF | SH_INNER | INDX_NORMAL_WB | AP_RW_EL1 | DESC_BLOCK;
    
    // Unused
    l1_page_table[2] = DESC_INVALID;

    // MMIO
    const uint64_t mmio_block_base = MMIO_BASE & ~(BLOCK_SIZE - 1);
    l1_page_table[3] = PXN | UXN | mmio_block_base | AF | INDX_DEVICE | AP_RW_EL1 | DESC_BLOCK;
  }

  // Configure MAIR_EL1 with defined attributes
  uint64_t mair = MAIR_DEVICE_nGnRnE | MAIR_NORMAL_WB | MAIR_NORMAL_NC;
  __asm__ __volatile__ ("msr mair_el1, %0" :: "r"(mair));

  // We have VAs until 0xFFFFFFFF = 32-bit address space
  __asm__ __volatile__ ("msr tcr_el1, %0" :: "r"(32UL));

  // Load L1 page table
  __asm__ __volatile__ ("msr ttbr0_el1, %0" :: "r"((uint64_t)l1_page_table));

  // Barriers
  __asm__ __volatile__ ("dsb sy; isb");

  // Enable MMU
  uint64_t sctlr;
  __asm__ __volatile__ ("mrs %0, sctlr_el1" : "=r"(sctlr));
  sctlr |= (1 << 0) | (1 << 2) | (1 << 12);
  __asm__ __volatile__ ("msr sctlr_el1, %0" :: "r"(sctlr));
  __asm__ __volatile__ ("dsb sy; isb");
}