#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*EndIRQCallback)(uint32_t);

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback);
int sched_start(void);
bool sched_create_task(void (*task_func)(void));
void sched_timer_irq_handler(uint32_t intid);
void print_task_ctx(uint64_t id);

#endif /* SCHED_H */