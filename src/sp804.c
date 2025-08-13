#include "sp804.h"
#include "platform.h"

static inline void sp804_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(ARM_TIMER_BASE + offset) = val;
}

static inline uint32_t sp804_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(ARM_TIMER_BASE + offset);
}

void sp804_enable(void) {
  sp804_write_reg32(CONTROL, sp804_read_reg32(CONTROL) | CONTROL_ENABLE);
}

void sp804_enable_free_counter(void) {
  sp804_write_reg32(CONTROL, sp804_read_reg32(CONTROL) | CONTROL_ENAFREE);
}

uint32_t sp804_get_free_count(void) {
  return sp804_read_reg32(FREECN);
}
