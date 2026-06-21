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
#define IDLE_TASK_INDEX MAX_TASKS  // Idle tasks are placed at the end of the task list

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
#define INITIAL_JUMP_TO_KERNEL_TASK(task_ctx, param) \
  do { \
    __asm__ __volatile__ ( \
      "mov sp, %0\n" \
      "mov x0, %1\n" \
      "br %2\n" \
      : \
      : "r" (task_ctx.sp_el1), "r" (param), "r" (task_ctx.pc) \
    ); \
    __builtin_unreachable(); \
  } while (0)

// Switch context from IRQ context to task that has been initialized but not run yet
#define INITIAL_JUMP_TO_KERNEL_TASK_FROM_IRQ(task_ctx, param) \
  do { \
    __asm__ __volatile__ ( \
      "mrs x0, spsr_el1\n" \
      "bic x0, x0, #0xF\n" \
      "mov x1, #0x5\n" \
      "orr x0, x0, x1\n" \
      "msr spsr_el1, x0\n" \
      "mov sp, %0\n" \
      "msr elr_el1, %2\n" \
      "mov x0, %1\n" \
      "eret\n" \
      : \
      : "r" (task_ctx.sp_el1), "r" (param), "r" (task_ctx.pc) \
      : "x0", "x1", "memory" \
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
      "mrs x0, spsr_el1\n" \
      "bic x0, x0, #0xF\n" \
      "mov x1, #0x5\n" \
      "orr x0, x0, x1\n" \
      "msr spsr_el1, x0\n" \
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
      : "x0", "x1", "memory" \
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
      : "x0", "memory" \
    ); \
    __builtin_unreachable(); \
  } while (0)

typedef enum {
  TASK_STATE_NONE, // Not allocated or terminated
  TASK_STATE_READY,
  TASK_STATE_RUNNING,
  TASK_STATE_BLOCKED,
  TASK_STATE_INITIAL, // Ready but never run before
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
  int index;
  TaskState state;
  TaskContext ctx;
  void *param; // Parameter for kernel task function
  uint64_t sleep_until;  // Timer count value when task should wake up
  uint32_t cpu_id;
  TaskType type;
  uint64_t* l2_table;
  pid_t pid;  // pid of corresponding user process, 0 if not user task
  
  uint8_t pre_stack_padding[256];
  __attribute__((aligned(16))) // AArch64 requires 16-byte alignment
  uint8_t stack[TASK_STACK_SIZE];
  uint8_t post_stack_padding[256];

} Task;

typedef struct SchedContext {
  bool initialized;
  Spinlock lock;
  uint32_t current_task_count;
  uint32_t total_tasks_created;
  Task* current_task[NUM_CPUS];
  uint64_t time_slice_cntp_tval;
  EndIRQCallback end_irq_callback;
  Task task_list[MAX_TASKS + NUM_CPUS];  // Last NUM_CPUS tasks are reserved for idle tasks, one per CPU
} SchedContext;


SchedContext sched_ctx;

static inline void lock_sched_ctx(void) {
  spinlock_acquire(&sched_ctx.lock);
}

static inline void unlock_sched_ctx(void) {
  spinlock_release(&sched_ctx.lock);
}

static Task* allocate_task(void) {
  for (uint32_t i = 0; i < MAX_TASKS; i++) {
    if (sched_ctx.task_list[i].state == TASK_STATE_NONE) {
      return &sched_ctx.task_list[i];
    }
  }
  return NULL;
}

static int free_task(Task* task) {
  if (task == NULL) {
    return -1;
  }

  int index = task->index;
  memset(task, 0, sizeof(Task));

  // Restore index
  task->index = index;

  return 0;
}

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

static inline Task* get_task_by_index(int index) {
  if (index < 0 || index >= (MAX_TASKS + NUM_CPUS)) {
    return NULL;
  }
  return &sched_ctx.task_list[index];
}

static inline Task* get_task_by_id(task_id_t id) {
  if (id < 0) {
    return NULL;
  }
  for (uint32_t i = 0; i < MAX_TASKS; i++) {
    if (sched_ctx.task_list[i].id == id) {
      return &sched_ctx.task_list[i];
    }
  }
  return NULL;
}

static inline Task* get_cpu_current_task(void) {
  return sched_ctx.current_task[GET_CPU_ID()];
}

// CPU core specific
static inline void set_cpu_current_task(Task* task) {
  sched_ctx.current_task[GET_CPU_ID()] = task;
}

static inline Task* get_cpu_idle_task(void) {
  return get_task_by_index(IDLE_TASK_INDEX + GET_CPU_ID());
}

pid_t sched_get_pid_by_task_id(task_id_t task_id) {
  Task* task = get_task_by_id(task_id);
  if (task == NULL) {
    return -1;
  }
  if (task->type != TASK_TYPE_USER) {
    return -2;
  }
  return task->pid;
}

static void create_idle_tasks(void) {
  for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
    Task* task = &sched_ctx.task_list[IDLE_TASK_INDEX + cpu];
    sched_ctx.current_task_count++;
    sched_ctx.total_tasks_created++;

    task->id = sched_ctx.total_tasks_created;
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

static Task* determine_cpu_next_task(void) {
  // Simple FIFO: find the next task in ready state by looping through all tasks
  uint32_t orig_task_idx = get_cpu_current_task()->index;

  uint32_t task_idx = orig_task_idx;
  for (uint32_t i = 0; i < MAX_TASKS; i++) {
    task_idx = (task_idx + 1) % MAX_TASKS;
    if (is_schedulable(&sched_ctx.task_list[task_idx])) {
      return &sched_ctx.task_list[task_idx];
    }
  }

  // Check if original task is still schedulable
  if (is_schedulable(&sched_ctx.task_list[orig_task_idx])) {
    return &sched_ctx.task_list[orig_task_idx];
  }

  return get_cpu_idle_task(); // No tasks ready, schedule idle task
}

// Switch context to new_task, calls IRQ end callback with int_id and cpu_id
// Assumes sched_ctx lock is held
static void switch_context_from_irq(Task* new_task, uint32_t int_id, uint32_t cpu_id) {
  bool initial = (new_task->state == TASK_STATE_INITIAL);
  bool user_task = (new_task->type == TASK_TYPE_USER);
  
  new_task->state = TASK_STATE_RUNNING;
  sched_ctx.end_irq_callback(int_id, cpu_id);

  start_timer();

  if (initial) {
    if (user_task) {
      mmu_set_user_l2_table(new_task->l2_table);
      unlock_sched_ctx();
      INITIAL_JUMP_TO_USER_TASK_FROM_IRQ(new_task->ctx);
    } else {
      mmu_set_user_l2_table(NULL);
      unlock_sched_ctx();
      INITIAL_JUMP_TO_KERNEL_TASK_FROM_IRQ(new_task->ctx, new_task->param);
    }
  }
  else {
    if (user_task) {
      mmu_set_user_l2_table(new_task->l2_table);
      unlock_sched_ctx();
      RESTORE_USER_CONTEXT_FROM_IRQ(new_task->ctx);
    } else {
      mmu_set_user_l2_table(NULL);
      unlock_sched_ctx();
      RESTORE_KERNEL_CONTEXT_FROM_IRQ(new_task->ctx);
    }
  }
}

void sched_init(uint64_t time_slice_us, EndIRQCallback end_irq_callback) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  sched_ctx.time_slice_cntp_tval = US_TO_CNTP_TVAL(time_slice_us);
  sched_ctx.end_irq_callback = end_irq_callback;
  create_idle_tasks();

  // Initialize task list indices
  for (uint32_t i = 0; i < MAX_TASKS + NUM_CPUS; i++) {
    sched_ctx.task_list[i].index = i;
  }

  sched_ctx.initialized = true;
}

int sched_start(void) {
  if (sched_ctx.initialized == false) {
    return -1;
  }

  // No lock needed because this is CPU specific

  // Start with idle task
  set_cpu_current_task(get_cpu_idle_task());

  Task* task = get_cpu_current_task();
  task->state = TASK_STATE_RUNNING;

  LOG(LOG_SCHED "switch CPU%d: start -> %ld\r\n", GET_CPU_ID(), task->id);

  start_timer();

  INITIAL_JUMP_TO_KERNEL_TASK(task->ctx, task->param);

  __builtin_unreachable();
}

static void wake_up_tasks() {
  // Check for sleeping tasks that need to wake up
  uint64_t current_time = GET_TIMER_COUNT();
  for (int i = 0; i < MAX_TASKS; i++) {
    if (sched_ctx.task_list[i].state == TASK_STATE_BLOCKED &&
        sched_ctx.task_list[i].sleep_until > 0 &&
        current_time >= sched_ctx.task_list[i].sleep_until) {
      LOG(LOG_SCHED "wakeup: %ld\r\n", sched_ctx.task_list[i].id);
      sched_ctx.task_list[i].state = TASK_STATE_READY;
      sched_ctx.task_list[i].sleep_until = 0;
    }
  }
}

// Will lock sched_ctx, but won't unlock in case of context switch
void sched_timer_irq_handler(uint32_t int_id, uint32_t cpu_id, uintptr_t sp_after_ctx_save) {
  lock_sched_ctx();
  stop_timer();
  wake_up_tasks();

  Task* current_task = get_cpu_current_task();

  if (current_task->state == TASK_STATE_RUNNING) {
    current_task->state = TASK_STATE_READY;
  }

  Task* next_task = determine_cpu_next_task();
  LOG(LOG_SCHED "switch CPU%d: %ld -> %ld\r\n",
           cpu_id, current_task->id, next_task->id);

  if (next_task == get_cpu_current_task()) {
    start_timer();
    current_task->state = TASK_STATE_RUNNING;
    unlock_sched_ctx();
    return;  // No need to switch context if task didn't change
  }

  // Context switch needed, save the rest of the context (if current task is not terminated)
  // General purpose registers have been saved already to the stack
  if (current_task->state != TASK_STATE_NONE) {
    SAVE_ELR_EL1(current_task->ctx);
    LOG(LOG_SCHED "saved elr_el1: 0x%lx\r\n", current_task->ctx.pc);

    if (current_task->type == TASK_TYPE_USER) {
      // User stack pointer needs to be saved separately for restoring user context later
      SAVE_SP_EL0(current_task->ctx);
      LOG(LOG_SCHED "saved sp_el0: 0x%lx\r\n", current_task->ctx.sp_el0);
    }

    current_task->ctx.sp_el1 = sp_after_ctx_save;
  }
  
  set_cpu_current_task(next_task);

  switch_context_from_irq(next_task, int_id, cpu_id);
  __builtin_unreachable();
}

task_id_t sched_create_kernel_task(void (*task_func)(void*), void *param) {
  if (!sched_ctx.initialized || sched_ctx.current_task_count >= MAX_TASKS) {
    return NO_TASK;
  }

  lock_sched_ctx();
  
  Task* new_task = allocate_task();
  if (new_task == NULL) {
    unlock_sched_ctx();
    return NO_TASK;
  }

  sched_ctx.current_task_count++;
  sched_ctx.total_tasks_created++;

  // use total_tasks_created as ID to avoid reusing IDs of terminated tasks
  new_task->id = (task_id_t)sched_ctx.total_tasks_created;
  new_task->ctx.sp_el1 = (uintptr_t)&new_task->stack[TASK_STACK_SIZE];
  new_task->ctx.sp_el0 = 0;
  new_task->ctx.pc = (uintptr_t)task_func;
  new_task->param = param;
  new_task->state = TASK_STATE_INITIAL;
  new_task->sleep_until = 0;
  new_task->type = TASK_TYPE_KERNEL;
  new_task->cpu_id = GET_CPU_ID();
  unlock_sched_ctx();

  return new_task->id;
}

task_id_t sched_create_user_task(uintptr_t entry_point_va, uint64_t* l2_table, 
                                 uint32_t cpu_id, uintptr_t sp, pid_t pid) {
  if (!sched_ctx.initialized || sched_ctx.current_task_count >= MAX_TASKS) {
    return NO_TASK;
  }

  lock_sched_ctx();
  Task* new_task = allocate_task();
  if (new_task == NULL) {
    unlock_sched_ctx();
    return NO_TASK;
  }

  sched_ctx.current_task_count++;
  sched_ctx.total_tasks_created++;

  // use total_tasks_created as ID to avoid reusing IDs of terminated tasks
  new_task->id = (task_id_t)sched_ctx.total_tasks_created;
  new_task->ctx.sp_el0 = sp;
  // this will be used when storing/restoring context
  new_task->ctx.sp_el1 = 0;
  new_task->ctx.pc = entry_point_va;
  new_task->state = TASK_STATE_INITIAL;
  new_task->sleep_until = 0;
  new_task->type = TASK_TYPE_USER;
  new_task->l2_table = l2_table;
  new_task->cpu_id = cpu_id;
  new_task->pid = pid;
  unlock_sched_ctx();

  return new_task->id;
}

void sched_block_current_task(void) {
  lock_sched_ctx();
  get_cpu_current_task()->state = TASK_STATE_BLOCKED;
  unlock_sched_ctx();
  sched_yield();
}

void sched_unblock_task(task_id_t task_id) {
  Task* task = get_task_by_id(task_id);
  if (task != NULL && task->state == TASK_STATE_BLOCKED) {
    task->state = TASK_STATE_READY;
    task->sleep_until = 0UL;
  }
}

void sched_block_task(task_id_t task_id) {
  Task* task = get_task_by_id(task_id);
  // Note: trying to block task that is in intital state might cause problems
  if (task != NULL && (task->state == TASK_STATE_RUNNING || task->state == TASK_STATE_READY)) {
    task->state = TASK_STATE_BLOCKED;
  }
}

// Works only for task on caller CPU
static inline uint64_t calculate_wakeup_ticks(uint64_t sleep_time_us) {
  uint64_t current_ticks = GET_TIMER_COUNT();
  uint64_t sleep_ticks = US_TO_CNTP_TVAL(sleep_time_us);
  return current_ticks + sleep_ticks;
}

void sched_sleep_cpu_current_task(uint64_t sleep_us) {
  if (sleep_us == 0) {
    sched_yield();
    return;
  }

  Task* current_task = get_cpu_current_task();

  lock_sched_ctx();
  current_task->sleep_until = calculate_wakeup_ticks(sleep_us);
  current_task->state = TASK_STATE_BLOCKED;
  unlock_sched_ctx();
  sched_yield();
}

void sched_yield(void) {
  trigger_timer_irq();
}

task_id_t sched_get_cpu_current_task_id(void) {
  Task* current_task = get_cpu_current_task();
  if (current_task == NULL) {
    return NO_TASK;
  }
  return current_task->id;
}

int sched_terminate_task(task_id_t task_id) {
  lock_sched_ctx();
  Task* task = get_task_by_id(task_id);
  if (task == NULL) {
    unlock_sched_ctx();
    return -1;
  }
  free_task(task);
  sched_ctx.current_task_count--;
  unlock_sched_ctx();

  return 0;
}


void sched_terminate_cpu_current_task(void) {
  Task* current_task = get_cpu_current_task();
  (void)sched_terminate_task(current_task->id);
  sched_yield();
  
  while (1) {
    WAIT_FOR_INTERRUPT();
  }
}

static inline void copy_saved_context(Task* dest, Task* src) {
  // Saved context from dest sp_el1 needs to be copied to src task stack
  uintptr_t dest_stack_top = (uintptr_t)&dest->stack[TASK_STACK_SIZE];
  const size_t offset = 31 * 8;
  
  uint64_t* context_in_dest_stack = (uint64_t*)(dest_stack_top - offset);
  uint64_t* context_in_src_stack = (uint64_t*)(src->ctx.sp_el1);

  // Copy general purpose registers x0-x30 (31 registers) that were saved to the stack during IRQ
  for (int i = 0; i < 31; i++) {
    context_in_dest_stack[i] = context_in_src_stack[i];
  }

  dest->ctx.sp_el1 = (uintptr_t)context_in_dest_stack;
}

task_id_t sched_clone_user_task(task_id_t src_task_id, uint64_t* l2_table, pid_t pid, uint32_t target_cpu) {
  Task* src_task = get_task_by_id(src_task_id);
  if (src_task == NULL || src_task->type != TASK_TYPE_USER) {
    return NO_TASK;
  }

  task_id_t new_task_id = sched_create_user_task(src_task->ctx.pc, l2_table, target_cpu,
                                                 src_task->ctx.sp_el0, pid);
  if (new_task_id == NO_TASK) {
    return NO_TASK;
  }
  Task* new_task = get_task_by_id(new_task_id);

  new_task->state = TASK_STATE_READY;
  copy_saved_context(new_task, src_task);

  return new_task_id;
}
