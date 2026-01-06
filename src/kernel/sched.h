#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 32

typedef int64_t task_id_t;
#define NO_TASK ((task_id_t)(-1))

typedef void (*EndIRQCallback)(uint32_t, uint32_t);

// TODO: refactor function return values

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback);
task_id_t sched_create_task(void (*task_func)(void));
int sched_start(void);
void sched_timer_irq_handler(uint32_t int_id, uint32_t cpu_id, uintptr_t sp_after_ctx_save);
task_id_t sched_get_task_id(void);
void sched_block_task(void);
void sched_unblock_task(task_id_t task_id);
void sched_yield(void);
void sched_sleep(uint64_t sleep_us);

#endif /* SCHED_H */