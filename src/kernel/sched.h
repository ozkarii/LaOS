#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 32

typedef int64_t task_id_t;
#define NO_TASK ((task_id_t)(-1))

typedef void (*EndIRQCallback)(uint32_t, uint32_t);

typedef enum {
  TASK_TYPE_KERNEL,
  TASK_TYPE_USER
} TaskType;

// TODO: refactor function return values

// Call only from one CPU
void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback);

// Create kernel task for caller CPU
task_id_t sched_create_kernel_task(void (*task_func)(void));
// Create user task for specified CPU
task_id_t sched_create_user_task(uintptr_t entry_point_va, uint64_t* l2_table, uint32_t cpu_id, uintptr_t sp);

// Call for each CPU
int sched_start(void);

// Call only from IRQ context
void sched_timer_irq_handler(uint32_t int_id, uint32_t cpu_id, uintptr_t sp_after_ctx_save);

// Block indefinitely until sched_unblock_task is called with the task ID
void sched_block_task(void);
// Unblock task with specified ID, making it eligible for scheduling again
void sched_unblock_task(task_id_t task_id);

// Yield the CPU to allow other tasks to run, but don't block the current task
void sched_yield(void);

// Sleep for approximately the specified number of microseconds, blocking the current task
void sched_sleep(uint64_t sleep_us);

// Get the ID of the task currently running on the calling CPU
task_id_t sched_get_task_id(void);

#endif /* SCHED_H */