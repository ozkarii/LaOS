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

#if ENABLE_LOG
#define LOG(...) \
    do { \
      k_printf(__VA_ARGS__); \
    } while (0)
#else
#define LOG(...) \
    do {} while (0);
#endif

// Loads target process's L2 table
#define WITH_PROCESS_L2_TABLE(pid, code_block) \
  do { \
    sched_set_l2_table_for_cpu_current_task(process_get_l2_table(pid)); \
    process_load_l2_table(pid); \
    code_block; \
    sched_set_l2_table_for_cpu_current_task(NULL); \
    process_unload_l2_table(pid); \
  } while (0)

typedef struct SyscallContext {
  pid_t pid;
  task_id_t task_id;
  long args[MAX_SYSCALL_PARAMS];
  long* ret;
} SyscallContext;

typedef void (*syscall_handler_fn)(SyscallContext*);

static inline void end_deferred_syscall_handler(SyscallContext *ctx) {
  LOG(LOG_SYSCALL "Ending syscall handler for syscall number %ld for PID %d\n", ctx->args[0], ctx->pid);
  sched_unblock_task(ctx->task_id);
  k_free(ctx);
  sched_terminate_cpu_current_task();
  __builtin_unreachable();
}

void handle_getpid(SyscallContext *ctx) {
  WITH_PROCESS_L2_TABLE(ctx->pid, {
    WRITE_AS_EL0_64(ctx->ret, ctx->pid);
  });
  end_deferred_syscall_handler(ctx);
}

void handle_write(SyscallContext *ctx) {
  int fd = ctx->args[0];
  char* user_buffer = (char*)ctx->args[1];
  size_t size = ctx->args[2];

  char* tmp_buffer = k_malloc(size + 1);

  WITH_PROCESS_L2_TABLE(ctx->pid, {
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
  });

  end_deferred_syscall_handler(ctx);
}

void handle_open(SyscallContext *ctx) {
  char* user_path = (char*)ctx->args[0];
  int flags = ctx->args[1];
  mode_t mode = ctx->args[2];

  char tmp_path[256];

  WITH_PROCESS_L2_TABLE(ctx->pid, {
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
  });
  end_deferred_syscall_handler(ctx);
}

void handle_read(SyscallContext *ctx) {
  int fd = ctx->args[0];
  char* user_buffer = (char*)ctx->args[1];
  size_t size = ctx->args[2];

  char* tmp_buffer = k_malloc(size);

  WITH_PROCESS_L2_TABLE(ctx->pid, {
    ssize_t bytes_read = process_read_file(ctx->pid, fd, tmp_buffer, size);

    if (bytes_read > 0) {
      for (ssize_t i = 0; i < bytes_read; i++) {
        WRITE_AS_EL0_8((user_buffer + i), tmp_buffer[i]);
      }
    }

    WRITE_AS_EL0_64(ctx->ret, bytes_read);
  });
  end_deferred_syscall_handler(ctx);
}

void handle_close(SyscallContext *ctx) {
  int fd = ctx->args[0];

  WITH_PROCESS_L2_TABLE(ctx->pid, {
    int ret = process_close_file(ctx->pid, fd);
    WRITE_AS_EL0_64(ctx->ret, ret);
  });
  end_deferred_syscall_handler(ctx);
}

void handle_exit(SyscallContext *ctx) {
  int status = ctx->args[0];
  
  LOG(LOG_SYSCALL "Process %d exited with status %d\n", ctx->pid, status);

  (void)status;
  (void)process_destroy(ctx->pid);

  end_deferred_syscall_handler(ctx);
}

void handle_execv(SyscallContext* ctx) {
  uint64_t* l2_table = process_get_l2_table(ctx->pid);
  sched_set_l2_table_for_cpu_current_task(l2_table);
  process_load_l2_table(ctx->pid);
  int ret = process_replace_executable(ctx->pid, (const char*)ctx->args[0]);
  if (ret == 0) {
    // If execv is successful, the process image is replaced, and we should not return to the old context.
    // Instead, we should terminate the current task and switch to the new executable.
    k_free(ctx);
    sched_terminate_cpu_current_task();
    __builtin_unreachable();
  }
  WRITE_AS_EL0_64(ctx->ret, ret);
  sched_set_l2_table_for_cpu_current_task(NULL);
  process_unload_l2_table(ctx->pid);
  end_deferred_syscall_handler(ctx);
}

void handle_fork(SyscallContext *ctx) {
  pid_t child_pid = process_clone(ctx->pid);

  WITH_PROCESS_L2_TABLE(ctx->pid, {
    WRITE_AS_EL0_64(ctx->ret, child_pid);
  });

  if (child_pid != -1) {
    // In the child process, return 0
    WITH_PROCESS_L2_TABLE(child_pid, {
      WRITE_AS_EL0_64(ctx->ret, 0);
    });
  }

  end_deferred_syscall_handler(ctx);
}

void handle_sleep(SyscallContext *ctx) {
  uint64_t sleep_us = (uint64_t)ctx->args[0];
  sched_sleep_cpu_current_task(sleep_us);
  ctx->ret = 0;
}

void handle_mkdir(SyscallContext *ctx) {
  WITH_PROCESS_L2_TABLE(ctx->pid, {
    int ret = vfs_mkdir((const char*)ctx->args[0], (int)ctx->args[1]);
    WRITE_AS_EL0_64(ctx->ret, ret);
  });

  end_deferred_syscall_handler(ctx);
}

void handle_unimplemented_syscall(SyscallContext *ctx) {
  LOG(LOG_SYSCALL "Unimplemented syscall number %ld for PID %d\n", ctx->args[0], ctx->pid);
  WRITE_AS_EL0_64(ctx->ret, -1);
}

typedef struct SyscallHandlerEntry {
  syscall_handler_fn fn;
  bool deferred;
} SyscallHandlerEntry;

static SyscallHandlerEntry syscall_handler_table[] = {
  [SYS_GETPID] = {handle_getpid, true},
  [SYS_OPEN] = {handle_open, true},
  [SYS_WRITE] = {handle_write, true},
  [SYS_READ] = {handle_read, true},
  [SYS_CLOSE] = {handle_close, true},
  [SYS_EXIT] = {handle_exit, true},
  [SYS_FORK] = {handle_fork, true},
  [SYS_EXECV] = {handle_execv, true},
  [SYS_SLEEP] = {handle_sleep, false},
  [SYS_MKDIR] = {handle_mkdir, true},
  [SYS_REMOVE] = {handle_unimplemented_syscall, false},
  [SYS_STAT] = {handle_unimplemented_syscall, false},
};

int syscall_handler(long number, long* ret, ...) {
  SyscallContext *ctx = k_malloc_irq_safe(sizeof(SyscallContext));
  SyscallHandlerEntry *handler_entry = &syscall_handler_table[number];

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
  if (handler_entry->deferred == false) {
    handler_entry->fn(ctx);
  } else {
    sched_create_kernel_task((void*)syscall_handler_table[number].fn, ctx);
    // This will generate timer irq right after returning to EL0
    sched_block_current_task();
  }

  return 0;
}