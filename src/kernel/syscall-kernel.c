#include <stdarg.h>

#include "string.h"

#include "syscall-kernel.h"
#include "sched.h"
#include "process.h"
#include "memory.h"
#include "log.h"
#include "armv8-a.h"
#include "vfs.h"
#include "io.h"

#define ENABLE_LOG 0

#ifdef ENABLE_LOG
#define LOG(...) \
    do { \
      k_printf(__VA_ARGS__); \
    } while (0)
#else
#define LOG(...) \
    do {} while (0);
#endif

typedef void (*syscall_handler_fn)(SyscallContext*);

static inline void end_syscall_handler(SyscallContext *ctx) {
  LOG(LOG_SYSCALL "Ending syscall handler for syscall number %ld for PID %d\n", ctx->args[0], ctx->pid);
  sched_unblock_task(ctx->task_id);
  k_free(ctx);
  sched_terminate_cpu_current_task();
  __builtin_unreachable();
}

void handle_getpid(SyscallContext *ctx) {
  process_load_l2_table(ctx->pid);
  WRITE_AS_EL0_64(ctx->ret, ctx->pid);
  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

void handle_write(SyscallContext *ctx) {
  int fd = ctx->args[0];
  char* user_buffer = (char*)ctx->args[1];
  size_t size = ctx->args[2];

  char* tmp_buffer = k_malloc(size + 1);

  process_load_l2_table(ctx->pid);
  
  for (size_t i = 0; i < size; i++) {
    tmp_buffer[i] = READ_AS_EL0_8((user_buffer + i));
  }
  tmp_buffer[size] = '\0';

  size_t bytes_written = 0;

  // "stdout"
  if (fd == 1) {
    k_puts(tmp_buffer);
    bytes_written = size;
  } else {
    bytes_written = process_write_file(ctx->pid, fd, tmp_buffer, size);
  }

  WRITE_AS_EL0_64(ctx->ret, bytes_written);

  process_unload_l2_table(ctx->pid);

  end_syscall_handler(ctx);
}

void handle_open(SyscallContext *ctx) {
  char* user_path = (char*)ctx->args[0];
  int flags = ctx->args[1];
  int mode = ctx->args[2];

  char tmp_path[256];

  process_load_l2_table(ctx->pid);
  
  for (size_t i = 0; i < sizeof(tmp_path) - 1; i++) {
    char c = READ_AS_EL0_8((user_path + i));
    tmp_path[i] = c;
    if (c == '\0') {
      break;
    }
  }
  tmp_path[sizeof(tmp_path) - 1] = '\0';

  int ret = process_open_file(ctx->pid, tmp_path, flags, mode);
  WRITE_AS_EL0_64(ctx->ret, ret);

  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

void handle_read(SyscallContext *ctx) {
  int fd = ctx->args[0];
  char* user_buffer = (char*)ctx->args[1];
  size_t size = ctx->args[2];

  char* tmp_buffer = k_malloc(size);

  process_load_l2_table(ctx->pid);
  
  ssize_t bytes_read = process_read_file(ctx->pid, fd, tmp_buffer, size);

  if (bytes_read > 0) {
    for (ssize_t i = 0; i < bytes_read; i++) {
      WRITE_AS_EL0_8((user_buffer + i), tmp_buffer[i]);
    }
  }

  WRITE_AS_EL0_64(ctx->ret, bytes_read);

  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

void handle_close(SyscallContext *ctx) {
  int fd = ctx->args[0];

  process_load_l2_table(ctx->pid);
  
  int ret = process_close_file(ctx->pid, fd);
  WRITE_AS_EL0_64(ctx->ret, ret);

  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

void handle_exit(SyscallContext *ctx) {
  int status = ctx->args[0];
  
  LOG(LOG_SYSCALL "Process %d exited with status %d\n", ctx->pid, status);

  (void)status;
  (void)process_destroy(ctx->pid);

  end_syscall_handler(ctx);
}

void handle_execv(SyscallContext* ctx) {
  process_load_l2_table(ctx->pid);
  WRITE_AS_EL0_64(ctx->ret, -1);
  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}


void handle_fork(SyscallContext *ctx) {
  pid_t child_pid = process_clone(ctx->pid);
  process_load_l2_table(ctx->pid);
  WRITE_AS_EL0_64(ctx->ret, child_pid);
  process_unload_l2_table(ctx->pid);
  end_syscall_handler(ctx);
}

static syscall_handler_fn syscall_handler_table[] = {
  [SYS_GETPID] = handle_getpid,
  [SYS_OPEN] = handle_open,
  [SYS_WRITE] = handle_write,
  [SYS_READ] = handle_read,
  [SYS_CLOSE] = handle_close,
  [SYS_EXIT] = handle_exit,
  [SYS_FORK] = handle_fork,
  [SYS_EXECV] = handle_execv
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

  LOG(LOG_SYSCALL "PID=%d task_id=%ld syscall_number=%ld, ret=%lx args=[%ld, %ld, %ld, %ld, %ld, %ld]\n", 
           ctx->pid, ctx->task_id, number, (uint64_t)ret, ctx->args[0], ctx->args[1], ctx->args[2], ctx->args[3], 
           ctx->args[4], ctx->args[5]);
  
  // Some syscalls can be handled without creating a handler task
  if (number == SYS_SLEEP) {
    sched_sleep_cpu_current_task((unsigned int)ctx->args[0] * 1000 * 1000);
    ctx->ret = 0;
  } else {
    sched_create_kernel_task((void*)syscall_handler_table[number], ctx);
    // This will generate timer irq right after returning to EL0
    sched_block_current_task();
  }

  return 0;
}