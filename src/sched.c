#include "string.h"
#include "armv8-a.h"
#include "sched.h"
#include "io.h"
#include "log.h"
#include "spinlock.h"

#define TASK_STACK_SIZE 0x4000  // 16 KB
#define US_TO_CNTP_TVAL(us) ((us) * GET_TIMER_FREQ() / 1000000ULL)
#define IDLE_TASK_INDEX MAX_TASKS  // Idle task has the highest task ID

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


// Restore full context from IRQ context and return from exception
// Assumes that task_ctx.sp points to the saved context on the stack,
// starting from x30 down to x0
// Jumps to task_ctx.pc with eret after restoring.
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
  uint64_t sleep_until;  // Timer count value when task should wake up
  
  uint8_t pre_stack_padding[256];
  __attribute__((aligned(16))) // AArch64 requires 16-byte alignment
  uint8_t stack[TASK_STACK_SIZE];
  uint8_t post_stack_padding[256];

} Task;

typedef struct {
  Spinlock lock;
  uint32_t task_count;
  uint32_t current_task;
  uint64_t time_slice_cntp_tval;
  EndIRQCallback end_irq_callback;
  Task task_list[MAX_TASKS + 1];
} SchedContext;


SchedContext sched_ctx;

// TODO: create interface with fn ptrs
static inline void stop_timer(void) {
  DISABLE_PHYS_TIMER();
}

static inline void start_timer(void) {
  SET_PHYS_TIMER_VALUE(sched_ctx.time_slice_cntp_tval);
  ENABLE_PHYS_TIMER();
}

static inline void trigger_timer_irq(void) {
  SET_PHYS_TIMER_VALUE(0);
}

__attribute__((unused))
void idle_task(void) {
  while (1) {
    __asm__ __volatile__ ("wfi");
  }
}

static void create_idle_task(void) {
  Task* task = &sched_ctx.task_list[IDLE_TASK_INDEX];
  task->id = (task_id_t)IDLE_TASK_INDEX;
  task->ctx.sp = (uintptr_t)&task->stack[TASK_STACK_SIZE];
  task->ctx.pc = (uintptr_t)idle_task;
  task->state = TASK_STATE_INITIAL;
  task->sleep_until = 0;
}

static inline bool is_schedulable_state(TaskState state) {
  return (state == TASK_STATE_READY || state == TASK_STATE_INITIAL);
}

static int64_t determine_next_task(void) {
  // Simple FIFO: find the next task in ready state by looping through all tasks
  uint32_t orig_task_idx = sched_ctx.current_task;

  uint32_t task_idx = orig_task_idx;
  for (uint32_t i = 0; i < MAX_TASKS; i++) {
    task_idx = (task_idx + 1) % MAX_TASKS;
    if (is_schedulable_state(sched_ctx.task_list[task_idx].state)) {
      return task_idx;
    }
  }

  // Check if original task is still schedulable
  if (is_schedulable_state(sched_ctx.task_list[orig_task_idx].state)) {
    return orig_task_idx;
  }

  return IDLE_TASK_INDEX;  // No tasks ready
}

// Switch context to new_task_idx, calls IRQ end callback with intid
static void switch_context_from_irq(Task* new_task, uint32_t intid) {
  bool initial = (new_task->state == TASK_STATE_INITIAL);
  
  new_task->state = TASK_STATE_RUNNING;
  sched_ctx.end_irq_callback(intid);

  start_timer();

  if (initial) {
    spinlock_release(&sched_ctx.lock);
    INITIAL_JUMP_TO_TASK_FROM_IRQ(new_task->ctx);
  }
  else {
    spinlock_release(&sched_ctx.lock);
    RESTORE_CONTEXT_FROM_IRQ(new_task->ctx);
  }
}

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  sched_ctx.time_slice_cntp_tval = US_TO_CNTP_TVAL(time_slice_us);
  sched_ctx.end_irq_callback = end_irq_callback;
  create_idle_task();
}

int sched_start(void) {
  start_timer();
  if (sched_ctx.task_count > 0) {
    sched_ctx.current_task = 0;
  } else {
    sched_ctx.current_task = IDLE_TASK_INDEX;
  }

  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_RUNNING;

  k_printf(LOG_SCHED "switch: start -> %ld\r\n",
           sched_ctx.task_list[sched_ctx.current_task].id);
  INITIAL_JUMP_TO_TASK(sched_ctx.task_list[sched_ctx.current_task].ctx);
  return 0;
}

static void wake_up_tasks() {
  // Check for sleeping tasks that need to wake up
  uint64_t current_time = GET_TIMER_COUNT();
  for (uint32_t i = 0; i < sched_ctx.task_count; i++) {
    if (sched_ctx.task_list[i].state == TASK_STATE_BLOCKED &&
        sched_ctx.task_list[i].sleep_until > 0 &&
        current_time >= sched_ctx.task_list[i].sleep_until) {
      k_printf(LOG_SCHED "wakeup: %ld\r\n", sched_ctx.task_list[i].id);
      sched_ctx.task_list[i].state = TASK_STATE_READY;
      sched_ctx.task_list[i].sleep_until = 0;
    }
  }
}

void sched_timer_irq_handler(uint32_t intid, uintptr_t sp_after_ctx_save) {
  spinlock_acquire(&sched_ctx.lock);
  stop_timer();
  wake_up_tasks();

  Task* current_task = &sched_ctx.task_list[sched_ctx.current_task];

  if (current_task->state == TASK_STATE_RUNNING) {
    current_task->state = TASK_STATE_READY;
  }

  int64_t next_task_idx = determine_next_task();
  k_printf(LOG_SCHED "switch: %ld -> %ld\r\n",
           current_task->id,
           sched_ctx.task_list[next_task_idx].id);

  if (next_task_idx == sched_ctx.current_task) {
    start_timer();
    current_task->state = TASK_STATE_RUNNING;
    spinlock_release(&sched_ctx.lock);
    return;  // No need to switch context if task didn't change
  }

  Task* next_task = &sched_ctx.task_list[next_task_idx];

  // Context switch needed, save the rest of the context
  // General purpose registers have been saved already to the stack
  SAVE_ELR_EL1(current_task->ctx);
  current_task->ctx.sp = sp_after_ctx_save;
  sched_ctx.current_task = next_task_idx;

  spinlock_release(&sched_ctx.lock);

  switch_context_from_irq(next_task, intid);
}

bool sched_create_task(void (*task_func)(void)) {
  if (sched_ctx.task_count >= MAX_TASKS) {
    return false;  // Max tasks reached
  }

  uint32_t new_task_idx = sched_ctx.task_count;
  Task* new_task = &sched_ctx.task_list[new_task_idx];

  spinlock_acquire(&sched_ctx.lock);
  sched_ctx.task_count++;
  new_task->id = (task_id_t)new_task_idx; // for now id == index
  new_task->ctx.sp = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.pc = (uint64_t)(uintptr_t)task_func;
  new_task->state = TASK_STATE_INITIAL;
  new_task->sleep_until = 0;
  spinlock_release(&sched_ctx.lock);

  return true;
}

task_id_t sched_get_task_id(void) {
  return sched_ctx.task_list[sched_ctx.current_task].id;
}

void sched_block_task(void) {
  spinlock_acquire(&sched_ctx.lock);
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_BLOCKED;
  spinlock_release(&sched_ctx.lock);
  trigger_timer_irq();
}

void sched_unblock_task(task_id_t task_id) {
  // for now, task_id == task_idx
  Task* task = &sched_ctx.task_list[task_id];
  if (task->state == TASK_STATE_BLOCKED) {
    task->state = TASK_STATE_READY;
    task->sleep_until = 0UL;
  }
}

void sched_sleep(uint64_t sleep_us) {
  if (sleep_us == 0) {
    sched_yield();
    return;
  }

  // Calculate wake-up time
  uint64_t current_time_ticks = GET_TIMER_COUNT();
  uint64_t sleep_ticks = US_TO_CNTP_TVAL(sleep_us);
  
  spinlock_acquire(&sched_ctx.lock);
  sched_ctx.task_list[sched_ctx.current_task].sleep_until = current_time_ticks + sleep_ticks;
  sched_ctx.task_list[sched_ctx.current_task].state = TASK_STATE_BLOCKED;
  spinlock_release(&sched_ctx.lock);
  trigger_timer_irq();
}

void sched_yield(void) {
  trigger_timer_irq();
}
