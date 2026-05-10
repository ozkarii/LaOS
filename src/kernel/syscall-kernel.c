#include <stdarg.h>

#include "string.h"

#include "syscall-kernel.h"
#include "syscall-common.h"
#include "sched.h"
#include "process.h"
#include "memory.h"
#include "log.h"

typedef long (*syscall_handler_fn)(void*);

syscall_handler_fn handle_exit(SyscallContext *ctx) {
  process_destroy(ctx->pid);
  ctx->ret = 0;
}

syscall_handler_fn handle_fork(SyscallContext *ctx) {
  ctx->ret = process_clone(ctx->pid);
}

syscall_handler_fn handle_getpid(SyscallContext *ctx) {
  ctx->ret = ctx->pid;
}

static syscall_handler_fn syscall_handler_table[] = {
  [SYS_EXIT] = handle_exit,
  [SYS_FORK] = handle_fork,
  [SYS_GETPID] = handle_getpid
};

long syscall_handler(long number, int num_args, ...) {
  SyscallContext *ctx = k_malloc(sizeof(SyscallContext));

  ctx->pid = sched_get_cpu_current_task_pid();

  va_list args;
  va_start(args, num_args);

  int i = 0;
  while (i < num_args) {
    ctx->args[i++] = va_arg(args, long);
  }
  while (i < 8) {
    ctx->args[i++] = 0;
  }

  k_printf(LOG_SYSCALL "PID=%d number=%ld, args=[%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld]\n", 
           ctx->pid, number, ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], 
           ctx->args[4], ctx->args[5], ctx->args[6], ctx->args[7]);

  sched_create_kernel_task((void*)syscall_handler_table[number], ctx);

  return ctx->ret;
}