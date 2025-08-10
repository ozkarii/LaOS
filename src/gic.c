#include <stdint.h>
#include "platform.h"
#include "gic.h"

static inline void gicd_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(GICD_BASE + offset) = val;
}

static inline uint32_t gicd_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(GICD_BASE + offset);
}

uint32_t gicd_get_type_reg(void) {
    return gicd_read_reg32(TYPER);
}