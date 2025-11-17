#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 16

typedef enum {
  TASK_STATE_READY,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_TERMINATED
} TaskState;

typedef struct {
  uint64_t sp;
  uint64_t pc;
  uint64_t x[31];
} TaskContext;

typedef struct {
  uint64_t id;
  TaskState state;
  TaskContext ctx;
} Task;

typedef struct {
  uint32_t task_count;
  uint32_t current_task;
  Task task_list[MAX_TASKS];
} SchedContext;


SchedContext sched_ctx;
uint64_t tasks_created = 0;

static inline void save_context(void) {
  __asm__ __volatile__ (
    "stp x0, x1, [%0, #16 * 0]\n"
    "stp x2, x3, [%0, #16 * 1]\n"
    "stp x4, x5, [%0, #16 * 2]\n"
    "stp x6, x7, [%0, #16 * 3]\n"
    "stp x8, x9, [%0, #16 * 4]\n"
    "stp x10, x11, [%0, #16 * 5]\n"
    "stp x12, x13, [%0, #16 * 6]\n"
    "stp x14, x15, [%0, #16 * 7]\n"
    "stp x16, x17, [%0, #16 * 8]\n"
    "stp x18, x19, [%0, #16 * 9]\n"
    "stp x20, x21, [%0, #16 * 10]\n"
    "stp x22, x23, [%0, #16 * 11]\n"
    "stp x24, x25, [%0, #16 * 12]\n"
    "stp x26, x27, [%0, #16 * 13]\n"
    "stp x28, x29, [%0, #16 * 14]\n"
    "str x30, [%0, #16 * 15]\n" // LR
    :
    : "r" (&sched_ctx.task_list[sched_ctx.current_task].ctx.x)
    : "memory"
  );

  __asm__ __volatile__ (
    "mov %0, sp\n"
    : "=r" (sched_ctx.task_list[sched_ctx.current_task].ctx.sp)
  );

  __asm__ __volatile__ (
    "adr %0, .\n"
    : "=r" (sched_ctx.task_list[sched_ctx.current_task].ctx.pc)
  );  
}

bool sched_init(void);
bool sched_create_task(void (*task_func)(void));
void sched_timer_irq_handler(void);
void sched_yield(void);



#endif /* SCHED_H */