#include "string.h"
#include "armv8-a.h"
#include "sched.h"
#include "io.h"

#define MAX_TASKS 16
#define TASK_STACK_SIZE 0x4000  // 16 KB
#define US_TO_CNTP_TVAL(us) ((us) * GET_TIMER_FREQ() / 1000000ULL)

#define SAVE_CONTEXT(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "stp x0, x1, [%0, #16 * 0]\n" \
      "stp x2, x3, [%0, #16 * 1]\n" \
      "stp x4, x5, [%0, #16 * 2]\n" \
      "stp x6, x7, [%0, #16 * 3]\n" \
      "stp x8, x9, [%0, #16 * 4]\n" \
      "stp x10, x11, [%0, #16 * 5]\n" \
      "stp x12, x13, [%0, #16 * 6]\n" \
      "stp x14, x15, [%0, #16 * 7]\n" \
      "stp x16, x17, [%0, #16 * 8]\n" \
      "stp x18, x19, [%0, #16 * 9]\n" \
      "stp x20, x21, [%0, #16 * 10]\n" \
      "stp x22, x23, [%0, #16 * 11]\n" \
      "stp x24, x25, [%0, #16 * 12]\n" \
      "stp x26, x27, [%0, #16 * 13]\n" \
      "stp x28, x29, [%0, #16 * 14]\n" \
      "str x30, [%0, #8 * 30]\n" \
      : \
      : "r" (task_ctx.x) \
      : "memory" \
    ); \
    __asm__ __volatile__ ( \
      "mov x0, sp\n" \
    ); \
    __asm__ __volatile__ ( \
      "str x0, [%0]\n" \
      : \
      : "r" (task_ctx.sp) \
      : "memory" \
    ); \
  } while (0)

#define JUMP_TO_ADDR(addr) \
  do { \
    __asm__ __volatile__ ( \
      "br %0\n" \
      : \
      : "r" (addr) \
    ); \
  } while (0)

#define RESTORE_CONTEXT(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "ldr x30, [%1, #8 * 30]\n" \
      "ldp x28, x29, [%1, #16 * 14]\n" \
      "ldp x26, x27, [%1, #16 * 13]\n" \
      "ldp x24, x25, [%1, #16 * 12]\n" \
      "ldp x22, x23, [%1, #16 * 11]\n" \
      "ldp x20, x21, [%1, #16 * 10]\n" \
      "ldp x18, x19, [%1, #16 * 9]\n" \
      "ldp x16, x17, [%1, #16 * 8]\n" \
      "ldp x14, x15, [%1, #16 * 7]\n" \
      "ldp x12, x13, [%1, #16 * 6]\n" \
      "ldp x10, x11, [%1, #16 * 5]\n" \
      "ldp x8, x9, [%1, #16 * 4]\n" \
      "ldp x6, x7, [%1, #16 * 3]\n" \
      "ldp x4, x5, [%1, #16 * 2]\n" \
      "ldp x2, x3, [%1, #16 * 1]\n" \
      "ldp x0, x1, [%1, #16 * 0]\n" \
      "br x30\n" \
      : \
      : "r" (task_ctx.sp), "r" (task_ctx.x) \
    ); \
    __builtin_unreachable(); \
  } while (0)


typedef enum {
  TASK_STATE_NONE, // If not allocated or terminated
  TASK_STATE_READY,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
} TaskState;

typedef struct {
  uintptr_t sp;
  uintptr_t pc;
  uint64_t x[31];
} TaskContext;

typedef struct {
  uint64_t id;
  TaskState state;
  TaskContext ctx;

  __attribute__((aligned(16)))
  uint8_t stack[TASK_STACK_SIZE];

} Task;

typedef struct {
  uint32_t task_count;
  uint32_t current_task;
  uint64_t time_slice_cntp_tval;
  Task task_list[MAX_TASKS];
} SchedContext;


SchedContext sched_ctx;


// TODO: create interface with fn ptrs
static void stop_timer(void) {
  DISABLE_PHYS_TIMER();
}

static void start_timer(void) {
  SET_PHYS_TIMER_VALUE(sched_ctx.time_slice_cntp_tval);
  ENABLE_PHYS_TIMER();
}

static void sched_idle(void) {
  while (1);
}

static int64_t determine_next_task(void) {
  // Simple FIFO: find the next task in ready state by looping through all tasks
  uint32_t orig_task_idx = sched_ctx.current_task; 
  uint32_t task_idx = orig_task_idx;

  // Loop until we find a ready task or wrap around back to original
  // TODO: use task_count to not loop over unused tasks
  do {
    task_idx = (task_idx + 1) % MAX_TASKS;
    if (sched_ctx.task_list[task_idx].state == TASK_STATE_READY) {
      return task_idx;
    }
  } while (task_idx != orig_task_idx);

  if (sched_ctx.task_list[orig_task_idx].state == TASK_STATE_READY) {
    return orig_task_idx;
  }

  return -1;  // No tasks ready, go idle or smth
}

static void switch_context(uint32_t new_task_idx) {
  sched_ctx.task_list[new_task_idx].state = TASK_STATE_RUNNING;
  start_timer();
  RESTORE_CONTEXT(sched_ctx.task_list[new_task_idx].ctx);
}

void sched_init(uint64_t time_slice_us) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  sched_ctx.time_slice_cntp_tval = US_TO_CNTP_TVAL(time_slice_us);
}

int sched_start(void) {
  start_timer();
  if (sched_ctx.task_count == 0) {
    return -1;
  }
  sched_ctx.current_task = 0;
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_RUNNING;
  RESTORE_CONTEXT(sched_ctx.task_list[0].ctx);
  return 0;
}

void sched_timer_irq_handler(void) {
  stop_timer();
  SAVE_CONTEXT(sched_ctx.task_list[sched_ctx.current_task].ctx);

  if (sched_ctx.task_list[sched_ctx.current_task].state == TASK_STATE_RUNNING) {
    sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_READY;
  }

  int64_t next_task_idx = determine_next_task();
  k_printf("%ld\n", next_task_idx);
  if (next_task_idx < 0) {
    start_timer();
    sched_idle();
  }
  else if (next_task_idx == sched_ctx.current_task) {
    start_timer();
    sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_RUNNING;
    return;  // No need to switch context if task didn't change
  }
  else {
    sched_ctx.current_task = next_task_idx;
  }

  switch_context(next_task_idx);
}

bool sched_create_task(void (*task_func)(void)) {
  uint32_t new_task_idx;

  if (sched_ctx.task_count >= MAX_TASKS) {
    return false;  // Max tasks reached
  }
  
  new_task_idx = sched_ctx.task_count++;

  Task* new_task = &sched_ctx.task_list[new_task_idx];

  new_task->ctx.sp = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.x[30] = (uint64_t)(uintptr_t)task_func;

  new_task->state = TASK_STATE_READY;

  return true;
}

