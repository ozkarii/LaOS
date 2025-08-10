#ifndef SYS_TIMER_H
#define SYS_TIMER_H

#include <stdint.h>

#define CLO 0x4
#define CHI 0x8

uint64_t sys_timer_get_value(void);

#endif // SYS_TIMER_H