#include "string.h"
#include "armv8-a.h"
#include "sched.h"
#include "io.h"

#define MAX_TASKS 32
#define TASK_STACK_SIZE 0x4000  // 16 KB
#define US_TO_CNTP_TVAL(us) ((us) * GET_TIMER_FREQ() / 1000000ULL)


// Switch context to initialized task (not from IRQ handler)
#define INITIAL_JUMP_TO_TASK(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "br %1\n" \
      : \
      : "r" (task_ctx.sp), "r" (task_ctx.pc) \
    ); \
    __builtin_unreachable(); \
  } while (0)

// Switch context to initialized task (from IRQ handler)
#define INITIAL_JUMP_TO_TASK_FROM_IRQ(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "msr elr_el1, %1\n" \
      "eret\n" \
      : \
      : "r" (task_ctx.sp), "r" (task_ctx.pc) \
    ); \
    __builtin_unreachable(); \
  } while (0)
  

#define SAVE_ELR_EL1(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mrs x0, elr_el1\n" \
      "str x0, [%0]\n" \
      : \
      : "r" (&task_ctx.pc) \
      : "x0", "memory" \
    ); \
  } while (0)

#define RESTORE_CONTEXT_FROM_IRQ(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "msr elr_el1, %0\n" \
      "mov sp, %1\n" \
      "ldr x30, [sp], #8\n" \
      "ldp x29, x28, [sp], #16\n" \
      "ldp x27, x26, [sp], #16\n" \
      "ldp x25, x24, [sp], #16\n" \
      "ldp x23, x22, [sp], #16\n" \
      "ldp x21, x20, [sp], #16\n" \
      "ldp x19, x18, [sp], #16\n" \
      "ldp x17, x16, [sp], #16\n" \
      "ldp x15, x14, [sp], #16\n" \
      "ldp x13, x12, [sp], #16\n" \
      "ldp x11, x10, [sp], #16\n" \
      "ldp x9, x8, [sp], #16\n" \
      "ldp x7, x6, [sp], #16\n" \
      "ldp x5, x4, [sp], #16\n" \
      "ldp x3, x2, [sp], #16\n" \
      "ldp x1, x0, [sp], #16\n" \
      "eret\n" \
      : \
      : "r" (task_ctx.pc), "r" (task_ctx.sp) \
    ); \
    __builtin_unreachable(); \
  } while (0)


typedef enum {
  TASK_STATE_NONE, // If not allocated or terminated
  TASK_STATE_READY,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_INITIAL
} TaskState;

typedef struct {
  uintptr_t sp;
  uintptr_t pc;
} TaskContext;

typedef struct {
  task_id_t id;
  TaskState state;
  TaskContext ctx;
  
  uint8_t pre_stack_padding[256];
  __attribute__((aligned(16))) // AArch64 requires 16-byte alignment
  uint8_t stack[TASK_STACK_SIZE];
  uint8_t post_stack_padding[256];

} Task;

typedef struct {
  uint32_t task_count;
  uint32_t current_task;
  uint64_t time_slice_cntp_tval;
  EndIRQCallback end_irq_callback;
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
  // Dummy
  while (1);
}

static inline bool is_schedulable_state(TaskState state) {
  return (state == TASK_STATE_READY || state == TASK_STATE_INITIAL);
}

static int64_t determine_next_task(void) {
  // Simple FIFO: find the next task in ready state by looping through all tasks
  uint32_t orig_task_idx = sched_ctx.current_task;
  uint32_t task_idx = orig_task_idx;

  // Loop until we find a ready task or wrap around back to original
  // TODO: use task_count to not loop over unused tasks
  do {
    task_idx = (task_idx + 1) % MAX_TASKS;
    if (is_schedulable_state(sched_ctx.task_list[task_idx].state)) {
      return task_idx;
    }
  } while (task_idx != orig_task_idx);

  // Check if original task is still schedulable
  if (is_schedulable_state(sched_ctx.task_list[orig_task_idx].state)) {
    return orig_task_idx;
  }

  return -1;  // No tasks ready, go idle or smth
}

// Switch context to new_task_idx, calls IRQ end callback with intid
static void switch_context(uint32_t new_task_idx, uint32_t intid) {
  bool initial = (sched_ctx.task_list[new_task_idx].state == TASK_STATE_INITIAL);
  sched_ctx.task_list[new_task_idx].state = TASK_STATE_RUNNING;
  sched_ctx.end_irq_callback(intid);

  start_timer();

  if (initial) {
    INITIAL_JUMP_TO_TASK_FROM_IRQ(sched_ctx.task_list[new_task_idx].ctx);
  } else {
    RESTORE_CONTEXT_FROM_IRQ(sched_ctx.task_list[new_task_idx].ctx);
  }
}

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  sched_ctx.time_slice_cntp_tval = US_TO_CNTP_TVAL(time_slice_us);
  sched_ctx.end_irq_callback = end_irq_callback;
}

int sched_start(void) {
  start_timer();
  if (sched_ctx.task_count == 0) {
    return -1;
  }
  sched_ctx.current_task = 0;
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_RUNNING;
  INITIAL_JUMP_TO_TASK(sched_ctx.task_list[0].ctx);
  return 0;
}

void sched_timer_irq_handler(uint32_t intid, uintptr_t sp_after_ctx_save) {
  stop_timer();

  if (sched_ctx.task_list[sched_ctx.current_task].state == TASK_STATE_RUNNING) {
    sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_READY;
  }

  int64_t next_task_idx = determine_next_task();
  k_printf("next_task_idx: %ld\n", next_task_idx);

  if (next_task_idx < 0) {
    start_timer();
    sched_ctx.end_irq_callback(intid);
    sched_idle();
  }
  else if (next_task_idx == sched_ctx.current_task) {
    start_timer();
    sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_RUNNING;
    return;  // No need to switch context if task didn't change
  }
  else {
    SAVE_ELR_EL1(sched_ctx.task_list[sched_ctx.current_task].ctx);
    sched_ctx.task_list[sched_ctx.current_task].ctx.sp = sp_after_ctx_save;
    sched_ctx.current_task = next_task_idx;
  }

  switch_context(next_task_idx, intid);
}

bool sched_create_task(void (*task_func)(void)) {
  uint32_t new_task_idx;

  if (sched_ctx.task_count >= MAX_TASKS) {
    return false;  // Max tasks reached
  }
  
  new_task_idx = sched_ctx.task_count++;

  Task* new_task = &sched_ctx.task_list[new_task_idx];

  new_task->ctx.sp = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.pc = (uint64_t)(uintptr_t)task_func;
  new_task->state = TASK_STATE_INITIAL;

  return true;
}
