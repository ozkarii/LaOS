#include <stdarg.h>

#include "string.h"

#include "syscall-kernel.h"
#include "sched.h"
#include "process.h"
#include "memory.h"
#include "log.h"
#include "armv8-a.h"


typedef long (*syscall_handler_fn)(void*);

static inline void end_syscall_handler(SyscallContext *ctx) {
  sched_unblock_task(ctx->task_id);
  k_free(ctx);
  sched_terminate_cpu_current_task();
}

syscall_handler_fn handle_exit(SyscallContext *ctx) {
  // TODO: implement process termination
}

syscall_handler_fn handle_fork(SyscallContext *ctx) {
  
}

syscall_handler_fn handle_getpid(SyscallContext *ctx) {
  process_load_l2_table(ctx->pid);
  WRITE_AS_EL0_64(ctx->ret, ctx->pid);
  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

syscall_handler_fn handle_write(SyscallContext *ctx) {
  char* user_buffer = (char*)ctx->args[1];
  size_t size = ctx->args[2];

  char* tmp_buffer = k_malloc(size + 1);

  process_load_l2_table(ctx->pid);
  
  for (size_t i = 0; i < size; i++) {
    tmp_buffer[i] = READ_AS_EL0_8((user_buffer + i));
  }
  tmp_buffer[size] = '\0';

  k_puts(tmp_buffer);

  WRITE_AS_EL0_64(ctx->ret, size);

  process_unload_l2_table(ctx->pid);

  end_syscall_handler(ctx);
}

static syscall_handler_fn syscall_handler_table[] = {
  [SYS_EXIT] = handle_exit,
  [SYS_FORK] = handle_fork,
  [SYS_GETPID] = handle_getpid,
  [SYS_WRITE] = handle_write
};

int syscall_handler(long number, long* ret, ...) {
  SyscallContext *ctx = k_malloc(sizeof(SyscallContext));

  ctx->task_id = sched_get_cpu_current_task_id();
  ctx->pid = sched_get_pid_by_task_id(ctx->task_id);

  va_list args;
  va_start(args, ret);

  // Warning: the args not used by this syscall will contain garbage
  int i = 0;
  while (i < MAX_SYSCALL_PARAMS) {
    ctx->args[i++] = va_arg(args, long);
  }

  // This is return value pointer owned by the user process
  ctx->ret = ret;

  k_printf(LOG_SYSCALL "PID=%d task_id=%ld syscall_number=%ld, ret=%lx args=[%ld, %ld, %ld, %ld, %ld, %ld]\n", 
           ctx->pid, ctx->task_id, number, ret, ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], 
           ctx->args[4], ctx->args[5]);

  sched_create_kernel_task((void*)syscall_handler_table[number], ctx);
  
  // This will generate timer irq right after returning to EL0
  sched_block_current_task();

  return 0;
}