#include "sys-timer.h"
#include "platform.h"

static inline void sys_timer_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(SYS_TIMER_BASE + offset) = val;
}

static inline uint32_t sys_timer_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(SYS_TIMER_BASE + offset);
}

uint64_t sys_timer_get_value(void) {
  // Using 32-bit accesses to be safe for now
  return (((uint64_t)sys_timer_read_reg32(CHI) << 32u)
         | (uint64_t)sys_timer_read_reg32(CLO));
}

int sys_timer_set_comp(uint32_t comp_reg, uint32_t value) {
  if (comp_reg > 3) {
    return -1;
  }
  sys_timer_write_reg32(C0 + (comp_reg * 4), value);
  return 0;
}

int sys_timer_check_match(uint32_t comp_reg) {  
  if (comp_reg > 3) {
    return -1;
  }
  uint32_t cs = sys_timer_read_reg32(CS);
  return (cs & (CS_M0 << comp_reg)) ? 1 : 0;
}

int sys_timer_clear_match(uint32_t comp_reg) {
  if (comp_reg > 3) {
    return -1;
  }
  sys_timer_write_reg32(CS, sys_timer_read_reg32(CS) & (CS_M0 << comp_reg));
  return 0;
}