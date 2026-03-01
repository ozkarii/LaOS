/**
 * Scheduler
 * 
 * TODO
 * - Refactor id - idx
 * - Refactor task creation functions, currently duplicate code
 * - Add task termination and cleanup
 * - Split code into common and CPU-core-specific parts 
 */

#include "string.h"
#include "armv8-a.h"
#include "sched.h"
#include "io.h"
#include "log.h"
#include "spinlock.h"
#include "mmu.h"

#define TASK_STACK_SIZE 0x4000  // 16 KB
#define US_TO_CNTP_TVAL(us) ((us) * GET_TIMER_FREQ() / 1000000ULL)
#define IDLE_TASK_INDEX MAX_TASKS  // Idle task has the highest task ID

#define ENABLE_LOG 0
#if ENABLE_LOG
#define LOG(...) \
    do { \
      k_printf(__VA_ARGS__); \
    } while (0)
#else
#define LOG(...) \
    do {} while (0);
#endif

// Switch context to task that has been initialized but not run yet
#define INITIAL_JUMP_TO_TASK(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "br %1\n" \
      : \
      : "r" (task_ctx.sp_el1), "r" (task_ctx.pc) \
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
      : "r" (task_ctx.sp_el1), "r" (task_ctx.pc) \
    ); \
    __builtin_unreachable(); \
  } while (0)

#define INITIAL_JUMP_TO_USER_TASK_FROM_IRQ(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mrs x0, spsr_el1\n" \
      "bic x0, x0, #0xF\n"\
      "msr spsr_el1, x0\n" \
      "mov sp, %0\n" \
      "msr elr_el1, %1\n" \
      "msr sp_el0, %2\n" \
      "eret\n" \
      : \
      : "r" (task_ctx.sp_el1), "r" (task_ctx.pc), "r" (task_ctx.sp_el0) \
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

// Save user stack pointer to task context, used for restoring context on task switch
#define SAVE_SP_EL0(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mrs x0, sp_el0\n" \
      "str x0, [%0]\n" \
      : \
      : "r" (&task_ctx.sp_el0) \
      : "x0", "memory" \
    ); \
  } while (0)


// Restore full context from IRQ context and return from exception
// Assumes that task_ctx.sp_el1 points to the saved context on the stack,
// starting from x30 down to x0
// Jumps to task_ctx.pc with eret after restoring.
#define RESTORE_KERNEL_CONTEXT_FROM_IRQ(task_ctx) \
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
      : "r" (task_ctx.pc), "r" (task_ctx.sp_el1) \
    ); \
    __builtin_unreachable(); \
  } while (0)

#define RESTORE_USER_CONTEXT_FROM_IRQ(task_ctx) \
  do { \
    __asm__ __volatile__ ( \
      "mrs x0, spsr_el1\n" \
      "bic x0, x0, #0x0F\n"\
      "msr spsr_el1, x0\n" \
      "msr elr_el1, %0\n" \
      "msr sp_el0, %1\n" \
      "mov sp, %2\n" \
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
      : "r" (task_ctx.pc), "r" (task_ctx.sp_el0), "r" (task_ctx.sp_el1) \
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

typedef struct TaskContext {
  // Stack where context is saved during IRQ, used for restoring context on task switch
  uintptr_t sp_el1;
  // User stack pointer if user task
  uintptr_t sp_el0;
  // Program counter to jump to when starting the task for the first time or restoring context
  uintptr_t pc;
} TaskContext;

typedef struct Task {
  task_id_t id;
  TaskState state;
  TaskContext ctx;
  uint64_t sleep_until;  // Timer count value when task should wake up
  uint32_t cpu_id;
  TaskType type;
  uint64_t* l2_table;
  
  uint8_t pre_stack_padding[256];
  __attribute__((aligned(16))) // AArch64 requires 16-byte alignment
  uint8_t stack[TASK_STACK_SIZE];
  uint8_t post_stack_padding[256];

} Task;

typedef struct SchedContext {
  bool initialized;
  Spinlock lock;
  uint32_t task_count;
  uint32_t current_task[NUM_CPUS];
  uint64_t time_slice_cntp_tval;
  EndIRQCallback end_irq_callback;
  Task task_list[MAX_TASKS + NUM_CPUS];
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
    WAIT_FOR_INTERRUPT();
  }
}

static inline Task* get_cpu_current_task(void) {
  return &sched_ctx.task_list[sched_ctx.current_task[GET_CPU_ID()]];
}

static inline int64_t get_cpu_current_task_idx(void) {
  return sched_ctx.current_task[GET_CPU_ID()];
}

// CPU core specific
static inline void set_cpu_current_task_idx(int64_t idx) {
  sched_ctx.current_task[GET_CPU_ID()] = idx;
}

static inline int64_t get_cpu_idle_task_idx(void) {
  return IDLE_TASK_INDEX + GET_CPU_ID();
}


static void create_idle_tasks(void) {
  for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
    Task* task = &sched_ctx.task_list[IDLE_TASK_INDEX + cpu];
    task->id = (task_id_t)(IDLE_TASK_INDEX + cpu);
    task->ctx.sp_el1 = (uintptr_t)&task->stack[TASK_STACK_SIZE];
    task->ctx.sp_el0 = 0;
    task->ctx.pc = (uintptr_t)idle_task;
    task->state = TASK_STATE_INITIAL;
    task->sleep_until = 0;
    task->type = TASK_TYPE_KERNEL;
    task->cpu_id = cpu;
  }
}

static inline bool is_schedulable(Task* task) {
  return ((task->state == TASK_STATE_READY || task->state == TASK_STATE_INITIAL))
         && (task->cpu_id == (GET_CPU_ID()));
}

static int64_t determine_next_task(void) {
  // Simple FIFO: find the next task in ready state by looping through all tasks
  uint32_t orig_task_idx = get_cpu_current_task_idx();

  uint32_t task_idx = orig_task_idx;
  for (uint32_t i = 0; i < MAX_TASKS; i++) {
    task_idx = (task_idx + 1) % MAX_TASKS;
    if (is_schedulable(&sched_ctx.task_list[task_idx])) {
      return task_idx;
    }
  }

  // Check if original task is still schedulable
  if (is_schedulable(&sched_ctx.task_list[orig_task_idx])) {
    return orig_task_idx;
  }

  return IDLE_TASK_INDEX + (GET_MPIDR() &  0xFF);  // No tasks ready
}

// Switch context to new_task_idx, calls IRQ end callback with int_id and cpu_id
static void switch_context_from_irq(Task* new_task, uint32_t int_id, uint32_t cpu_id) {
  bool initial = (new_task->state == TASK_STATE_INITIAL);
  bool user_task = (new_task->type == TASK_TYPE_USER);
  
  new_task->state = TASK_STATE_RUNNING;
  sched_ctx.end_irq_callback(int_id, cpu_id);

  start_timer();

  if (initial) {
    if (user_task) {
      spinlock_release(&sched_ctx.lock);
      mmu_set_user_l2_table(new_task->l2_table);
      INITIAL_JUMP_TO_USER_TASK_FROM_IRQ(new_task->ctx);
    } else {
      spinlock_release(&sched_ctx.lock);
      mmu_set_user_l2_table(NULL);
      INITIAL_JUMP_TO_TASK_FROM_IRQ(new_task->ctx);
    }
  }
  else {
    if (user_task) {
      spinlock_release(&sched_ctx.lock);
      mmu_set_user_l2_table(new_task->l2_table);
      RESTORE_USER_CONTEXT_FROM_IRQ(new_task->ctx);
    } else {
      spinlock_release(&sched_ctx.lock);
      mmu_set_user_l2_table(NULL);
      RESTORE_KERNEL_CONTEXT_FROM_IRQ(new_task->ctx);
    }
  }
}

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  sched_ctx.time_slice_cntp_tval = US_TO_CNTP_TVAL(time_slice_us);
  sched_ctx.end_irq_callback = end_irq_callback;
  create_idle_tasks();
  sched_ctx.initialized = true;
}

int sched_start(void) {
  if (sched_ctx.initialized == false) {
    return -1;
  }

  // No lock needed because this is CPU specific

  // Start with idle task
  set_cpu_current_task_idx(get_cpu_idle_task_idx());

  Task* task = get_cpu_current_task();
  task->state = TASK_STATE_RUNNING;

  LOG(LOG_SCHED "switch CPU%d: start -> %ld\r\n", GET_CPU_ID(), task->id);

  start_timer();

  INITIAL_JUMP_TO_TASK(task->ctx);

  __builtin_unreachable();
}

static void wake_up_tasks() {
  // Check for sleeping tasks that need to wake up
  uint64_t current_time = GET_TIMER_COUNT();
  for (uint32_t i = 0; i < sched_ctx.task_count; i++) {
    if (sched_ctx.task_list[i].state == TASK_STATE_BLOCKED &&
        sched_ctx.task_list[i].sleep_until > 0 &&
        current_time >= sched_ctx.task_list[i].sleep_until) {
      LOG(LOG_SCHED "wakeup: %ld\r\n", sched_ctx.task_list[i].id);
      sched_ctx.task_list[i].state = TASK_STATE_READY;
      sched_ctx.task_list[i].sleep_until = 0;
    }
  }
}

void sched_timer_irq_handler(uint32_t int_id, uint32_t cpu_id, uintptr_t sp_after_ctx_save) {
  spinlock_acquire(&sched_ctx.lock);
  stop_timer();
  wake_up_tasks();

  Task* current_task = get_cpu_current_task();

  if (current_task->state == TASK_STATE_RUNNING) {
    current_task->state = TASK_STATE_READY;
  }

  int64_t next_task_idx = determine_next_task();
  LOG(LOG_SCHED "switch CPU%d: %ld -> %ld\r\n",
           cpu_id, current_task->id,
           sched_ctx.task_list[next_task_idx].id);

  if (next_task_idx == get_cpu_current_task_idx()) {
    start_timer();
    current_task->state = TASK_STATE_RUNNING;
    spinlock_release(&sched_ctx.lock);
    return;  // No need to switch context if task didn't change
  }

  Task* next_task = &sched_ctx.task_list[next_task_idx];

  // Context switch needed, save the rest of the context
  // General purpose registers have been saved already to the stack
  SAVE_ELR_EL1(current_task->ctx);
  LOG(LOG_SCHED "saved elr_el1: 0x%lx\r\n", current_task->ctx.pc);

  if (current_task->type == TASK_TYPE_USER) {
    SAVE_SP_EL0(current_task->ctx);
    LOG(LOG_SCHED "saved sp_el0: 0x%lx\r\n", current_task->ctx.sp_el0);
  }

  current_task->ctx.sp_el1 = sp_after_ctx_save;
  set_cpu_current_task_idx(next_task_idx);

  switch_context_from_irq(next_task, int_id, cpu_id);
  __builtin_unreachable();
}

task_id_t sched_create_kernel_task(void (*task_func)(void)) {
  if (!sched_ctx.initialized || sched_ctx.task_count >= MAX_TASKS) {
    return NO_TASK;
  }

  spinlock_acquire(&sched_ctx.lock);
  uint32_t new_task_idx = sched_ctx.task_count;
  Task* new_task = &sched_ctx.task_list[new_task_idx];

  sched_ctx.task_count++;
  new_task->id = (task_id_t)new_task_idx; // for now id == index
  new_task->ctx.sp_el1 = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.sp_el0 = 0;
  new_task->ctx.pc = (uintptr_t)task_func;
  new_task->state = TASK_STATE_INITIAL;
  new_task->sleep_until = 0;
  new_task->type = TASK_TYPE_KERNEL;
  new_task->cpu_id = GET_CPU_ID();
  spinlock_release(&sched_ctx.lock);

  return new_task->id;
}

task_id_t sched_create_user_task(uintptr_t entry_point_va, uint64_t* l2_table, uint32_t cpu_id, uintptr_t sp) {
  if (!sched_ctx.initialized || sched_ctx.task_count >= MAX_TASKS) {
    return NO_TASK;
  }

  spinlock_acquire(&sched_ctx.lock);
  uint32_t new_task_idx = sched_ctx.task_count;
  Task* new_task = &sched_ctx.task_list[new_task_idx];

  sched_ctx.task_count++;
  new_task->id = (task_id_t)new_task_idx; // for now id == index
  new_task->ctx.sp_el0 = sp;
  // this will be used when storing/restoring context
  new_task->ctx.sp_el1 = 0;
  new_task->ctx.pc = entry_point_va;
  new_task->state = TASK_STATE_INITIAL;
  new_task->sleep_until = 0;
  new_task->type = TASK_TYPE_USER;
  new_task->l2_table = l2_table;
  new_task->cpu_id = cpu_id;
  spinlock_release(&sched_ctx.lock);

  return new_task->id;
}

task_id_t sched_get_task_id(void) {
  return sched_ctx.task_list[get_cpu_current_task_idx()].id;
}

void sched_block_task(void) {
  spinlock_acquire(&sched_ctx.lock);
  get_cpu_current_task()->state = TASK_STATE_BLOCKED;
  spinlock_release(&sched_ctx.lock);
  sched_yield();
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
  Task* current_task = get_cpu_current_task();
  
  spinlock_acquire(&sched_ctx.lock);
  current_task->sleep_until = current_time_ticks + sleep_ticks;
  current_task->state = TASK_STATE_BLOCKED;
  spinlock_release(&sched_ctx.lock);
  sched_yield();
}

void sched_yield(void) {
  trigger_timer_irq();
}
