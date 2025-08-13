#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include <stdint.h>

#define CS     0x00
#define CS_M0  1u
#define CS_M1  (1u << 1u)
#define CS_M2  (1u << 2u)
#define CS_M3  (1u << 3u)

#define CLO    0x04
#define CHI    0x08
#define C0     0x0c
#define C1     0x10
#define C2     0x14
#define C3     0x18


uint64_t sys_timer_get_value(void);
int sys_timer_set_comp(uint32_t comp_reg, uint32_t value);
int sys_timer_check_match(uint32_t comp_reg);
int sys_timer_clear_match(uint32_t comp_reg);

#endif // SYS_TIMER_H