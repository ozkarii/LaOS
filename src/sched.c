#include "string.h"
#include "armv8-a.h"
#include "sched.h"
#include "io.h"

#define MAX_TASKS 32
#define TASK_STACK_SIZE 0x4000  // 16 KB
#define US_TO_CNTP_TVAL(us) ((us) * GET_TIMER_FREQ() / 1000000ULL)


// Switch context to task that has been initialized but not run yet
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

// Switch context from IRQ context to task that has been initialized but not run yet
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
  

// Save exception link register (program counter to restore) to task context
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

// Saves context but x30 is replaced with the address of return_label.
// Address of return_label is stored in task_ctx.pc.
#define SAVE_CONTEXT_AND_LINK_TO_LABEL(task_ctx, return_label) \
  do { \
    __asm__ __volatile__ goto ( \
      "stp x0, x1, [sp, #-16]!\n" \
      "stp x2, x3, [sp, #-16]!\n" \
      "stp x4, x5, [sp, #-16]!\n" \
      "stp x6, x7, [sp, #-16]!\n" \
      "stp x8, x9, [sp, #-16]!\n" \
      "stp x10, x11, [sp, #-16]!\n" \
      "stp x12, x13, [sp, #-16]!\n" \
      "stp x14, x15, [sp, #-16]!\n" \
      "stp x16, x17, [sp, #-16]!\n" \
      "stp x18, x19, [sp, #-16]!\n" \
      "stp x20, x21, [sp, #-16]!\n" \
      "stp x22, x23, [sp, #-16]!\n" \
      "stp x24, x25, [sp, #-16]!\n" \
      "stp x26, x27, [sp, #-16]!\n" \
      "stp x28, x29, [sp, #-16]!\n" \
      "adr x0, %l[" #return_label "]\n" \
      "str x0, [%0]\n" \
      "mov x30, x0\n" \
      "str x30, [sp, #-8]!\n" \
      "mov x0, sp\n" \
      "str x0, [%1]\n" \
      : \
      : "r" (&task_ctx.pc), "r" (&task_ctx.sp) \
      : "x0", "memory" \
      : return_label \
    ); \
  } while (0)

// Restore full context from IRQ context and return from exception
// Assumes that task_ctx.sp points to the saved context on the stack,
// starting from x30 down to x0
#define RESTORE_CONTEXT_FROM_IRQ(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "msr elr_el1, %0\n" \
      "mov sp, %1\n" \
      "ldr x30, [sp], #8\n" \
      "ldp x28, x29, [sp], #16\n" \
      "ldp x26, x27, [sp], #16\n" \
      "ldp x24, x25, [sp], #16\n" \
      "ldp x22, x23, [sp], #16\n" \
      "ldp x20, x21, [sp], #16\n" \
      "ldp x18, x19, [sp], #16\n" \
      "ldp x16, x17, [sp], #16\n" \
      "ldp x14, x15, [sp], #16\n" \
      "ldp x12, x13, [sp], #16\n" \
      "ldp x10, x11, [sp], #16\n" \
      "ldp x8, x9, [sp], #16\n" \
      "ldp x6, x7, [sp], #16\n" \
      "ldp x4, x5, [sp], #16\n" \
      "ldp x2, x3, [sp], #16\n" \
      "ldp x0, x1, [sp], #16\n" \
      "eret\n" \
      : \
      : "r" (task_ctx.pc), "r" (task_ctx.sp) \
    ); \
    __builtin_unreachable(); \
  } while (0)

// Restore full context from non-IRQ context
// Assumes that task_ctx.sp points to the saved context on the stack,
// starting from x30 down to x0.
// Jumps to x30 (link register) after restoring.
#define RESTORE_CONTEXT(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "ldr x30, [sp], #8\n" \
      "ldp x28, x29, [sp], #16\n" \
      "ldp x26, x27, [sp], #16\n" \
      "ldp x24, x25, [sp], #16\n" \
      "ldp x22, x23, [sp], #16\n" \
      "ldp x20, x21, [sp], #16\n" \
      "ldp x18, x19, [sp], #16\n" \
      "ldp x16, x17, [sp], #16\n" \
      "ldp x14, x15, [sp], #16\n" \
      "ldp x12, x13, [sp], #16\n" \
      "ldp x10, x11, [sp], #16\n" \
      "ldp x8, x9, [sp], #16\n" \
      "ldp x6, x7, [sp], #16\n" \
      "ldp x4, x5, [sp], #16\n" \
      "ldp x2, x3, [sp], #16\n" \
      "ldp x0, x1, [sp], #16\n" \
      "br x30\n" \
      : \
      : "r" (task_ctx.sp) \
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
static void switch_context_from_irq(uint32_t new_task_idx, uint32_t intid) {
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

// Switch context to new_task_idx from non-IRQ context
static void switch_context(uint32_t new_task_idx) {
  bool initial = (sched_ctx.task_list[new_task_idx].state == TASK_STATE_INITIAL);
  sched_ctx.task_list[new_task_idx].state = TASK_STATE_RUNNING;

  start_timer();

  if (initial) {
    INITIAL_JUMP_TO_TASK(sched_ctx.task_list[new_task_idx].ctx);
  } else {
    RESTORE_CONTEXT(sched_ctx.task_list[new_task_idx].ctx);
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

  switch_context_from_irq(next_task_idx, intid);
}

bool sched_create_task(void (*task_func)(void)) {
  if (sched_ctx.task_count >= MAX_TASKS) {
    return false;  // Max tasks reached
  }

  uint32_t new_task_idx = sched_ctx.task_count++;
  Task* new_task = &sched_ctx.task_list[new_task_idx];

  new_task->id = (task_id_t)new_task_idx; // for now id == index
  new_task->ctx.sp = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.pc = (uint64_t)(uintptr_t)task_func;
  new_task->state = TASK_STATE_INITIAL;

  return true;
}

task_id_t sched_get_task_id(void) {
  return sched_ctx.task_list[sched_ctx.current_task].id;
}

void sched_block_task(void) {
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_BLOCKED;
}

static void sched_yield_inner(void) {
  stop_timer();
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_READY;

  int64_t next_task_idx = determine_next_task();
  if (next_task_idx < 0) {
    start_timer();
    sched_idle();
  }
  else {
    sched_ctx.current_task = next_task_idx;
  }

  switch_context(next_task_idx);
}

void sched_yield(void) {
  SAVE_CONTEXT_AND_LINK_TO_LABEL(sched_ctx.task_list[sched_ctx.current_task].ctx, sched_yield_return);
  sched_yield_inner();
sched_yield_return:
  return;
}
