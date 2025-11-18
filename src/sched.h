#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>


void sched_init(uint64_t time_slice_us);
int sched_start(void);
bool sched_create_task(void (*task_func)(void));
void sched_timer_irq_handler(void);

#endif /* SCHED_H */