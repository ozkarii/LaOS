#include "sys-timer.h"
#include "platform.h"

static inline void sys_timer_write_reg(uint64_t offset, uint32_t val) {
    *(volatile uint32_t*)(SYS_TIMER_BASE + offset) = val;
}

static inline uint32_t sys_timer_read_reg(uint64_t offset) {
    return *(volatile uint32_t*)(SYS_TIMER_BASE + offset);
}

uint64_t sys_timer_get_value(void) {
    // Using 32-bit accesses to be safe for now
    return (((uint64_t)sys_timer_read_reg(CHI) << 32u)
           | (uint64_t)sys_timer_read_reg(CLO));
}
