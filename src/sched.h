#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t task_id_t;
typedef void (*EndIRQCallback)(uint32_t);

// TODO: refactor function return values

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback);
bool sched_create_task(void (*task_func)(void));
int sched_start(void);
void sched_timer_irq_handler(uint32_t intid, uintptr_t sp_after_ctx_save);
task_id_t sched_get_task_id(void);
void sched_block_task(void);
void sched_unblock_task(task_id_t task_id);

#endif /* SCHED_H */