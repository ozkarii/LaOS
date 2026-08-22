#include "string.h"

#include "io.h"
#include "process.h"
#include "vfs.h"
#include "log.h"
#include "sched.h"
#include "memory.h"
#include "mmu.h"
#include "armv8-a.h"
#include "elf-loader.h"
#include "spinlock.h"

#define ENABLE_LOG 1

#if ENABLE_LOG
#define LOG(...) \
    do { \
      k_printf(__VA_ARGS__); \
    } while (0)
#else
#define LOG(...) \
    do {} while (0);
#endif

typedef struct FileDescriptor {
  int fd;
  VFSFileDescriptor* vfs_fd;
} FileDescriptor;

typedef struct Process {
  pid_t pid;
  task_id_t task_id;
  bool allocated;
  FileDescriptor open_fds[MAX_OPEN_FDS];
  uint64_t* l2_table;
  VirtualMemoryMapping virtual_memory_mappings[MAX_VIRTUAL_MEMORY_MAPPINGS];
} Process;

typedef struct ProcessesContext {
  Process processes[MAX_PROCESSES];
  pid_t pid_counter;
  Spinlock lock;
} ProcessesContext;


static ProcessesContext processes_ctx;

static inline void lock_processes_ctx(void) {
  spinlock_acquire(&processes_ctx.lock);
}

static inline void unlock_processes_ctx(void) {
  spinlock_release(&processes_ctx.lock);
}

static inline Process* get_process_by_pid(pid_t pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processes_ctx.processes[i].allocated && processes_ctx.processes[i].pid == pid) {
      return &processes_ctx.processes[i];
    }
  }
  return NULL;
}

// Process ctx lock must be held before calling this function
// Will not release the lock
static Process* create_process(void) {
  Process *p = NULL;

  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!processes_ctx.processes[i].allocated) {
      p = &processes_ctx.processes[i];
      break;
    }
  }
  if (p == NULL) {
    goto no_space;
  }

  p->allocated = true;
  p->l2_table = mmu_create_user_l2_table();

  if (p->l2_table == NULL) {
    goto mmu_create_user_l2_table_fail;
  }
  // First pid is 1
  p->pid = ++processes_ctx.pid_counter;

  return p;

mmu_create_user_l2_table_fail:
  p->allocated = false;
no_space:
  return NULL;
}

static int destroy_process_lock_held(Process* process) {
  if (process->allocated == false) {
    LOG(LOG_KERNEL "Attempted to destroy an unallocated process with pid %d\n", process->pid);
    return -1;
  }

  for (int j = 0; j < MAX_OPEN_FDS; j++) {
    if (process->open_fds[j].vfs_fd != NULL) {
      if (vfs_close(process->open_fds[j].vfs_fd) != 0) {
        k_printf(LOG_KERNEL "Failed to close fd %d for process %d during destruction\n",
                  process->open_fds[j].fd, process->pid);
      }
    }
  }

  for (int j = 0; j < MAX_VIRTUAL_MEMORY_MAPPINGS; j++) {
    if (process->virtual_memory_mappings[j].pa != NULL) {
      free_user_memory_block(&process->virtual_memory_mappings[j]);
    }
  }

  mmu_free_user_l2_table(process->l2_table);

  (void)sched_terminate_task(process->task_id);

  memset(process->open_fds, 0, sizeof(process->open_fds));
  memset(process->virtual_memory_mappings, 0, sizeof(process->virtual_memory_mappings));
  memset(process, 0, sizeof(Process));

  return 0;
}

int process_destroy(pid_t pid) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }

  lock_processes_ctx();
 
  if (destroy_process_lock_held(process) != 0) {
    unlock_processes_ctx();
    return -1;
  }

  unlock_processes_ctx();

  return 0;
}

pid_t process_create_init_process(void) {
  lock_processes_ctx();
  if (processes_ctx.pid_counter != 0) {
    return -1;
  }

  Process* p = create_process();

  if (p == NULL) {
    unlock_processes_ctx();
    return -1;
  }

  int ret = allocate_user_memory_block(p->l2_table, true, &p->virtual_memory_mappings[0]);
  if (ret != 0) {
    goto destroy_process;
  }

  ret = allocate_user_memory_block(p->l2_table, false, &p->virtual_memory_mappings[1]);
  if (ret != 0) {
    goto destroy_process;
  }


  VFSFileDescriptor* init_bin_fd = vfs_open("/sbin/init", O_RDONLY, 0);
  if (init_bin_fd == NULL) {
    goto destroy_process;
  }

  // Dummy console to reserve fd 1
  VFSFileDescriptor* console_fd = vfs_open("/dev/console", O_RDWR, 0);
  if (console_fd == NULL) {
    goto destroy_process;
  }

  p->open_fds[1].fd = 1;
  p->open_fds[1].vfs_fd = console_fd;

  VFSStat stat;
  if (vfs_stat("/sbin/init", &stat) != 0) {
    goto destroy_process;
  }

  void *tmp_elf = k_malloc(stat.size);
  if (tmp_elf == NULL) {
    goto destroy_process;
  }

  if (vfs_read(init_bin_fd, tmp_elf, stat.size) != (ssize_t)stat.size) {
    goto free_tmp_elf;
  }

  uint64_t entry_offset = 0;
  if (elf_load_from_memory(tmp_elf, stat.size, p->virtual_memory_mappings[0].pa,
                           p->virtual_memory_mappings[1].pa, &entry_offset) != 0) {
    goto free_tmp_elf;
  }

  task_id_t id = sched_create_user_task(entry_offset, p->l2_table, GET_CPU_ID(),
                                        STACK_TOP_VA, p->pid);
  if (id == NO_TASK) {
    goto free_tmp_elf;
  }
  p->task_id = id;

  vfs_close(init_bin_fd);

  unlock_processes_ctx();

  return p->pid;

free_tmp_elf:
  k_free(tmp_elf);
destroy_process:
  (void)destroy_process_lock_held(p);
  unlock_processes_ctx();
  return -1;
}

pid_t process_clone(pid_t parent_pid) {
  Process* parent = get_process_by_pid(parent_pid);

  if (parent == NULL) {
    return -1;
  }

  lock_processes_ctx();

  // Check if parent is still valid after acquiring the lock
  if (parent->allocated == false) {
    unlock_processes_ctx();
    return -1;
  }

  Process* child = create_process();

  if (child == NULL) {
    unlock_processes_ctx();
    return -1;
  }

  // Copy open fds and increment ref_count
  for (int i = 0; i < MAX_OPEN_FDS; i++) {
    if (parent->open_fds[i].vfs_fd != NULL) {
      child->open_fds[i] = parent->open_fds[i];
      child->open_fds[i].vfs_fd->ref_count++;
    }
  }

  // Copy virtual memory mappings and data
  for (int i = 0; i < MAX_VIRTUAL_MEMORY_MAPPINGS; i++) {
    VirtualMemoryMapping* parent_mapping = &parent->virtual_memory_mappings[i];
    if (parent_mapping->pa != NULL) {
      int ret = allocate_user_memory_block(child->l2_table, parent_mapping->executable,
                                           &child->virtual_memory_mappings[i]);
      if (ret != 0) {
        goto process_clone_error;
      }
      memcpy(child->virtual_memory_mappings[i].pa, parent_mapping->pa, parent_mapping->size);
    }
  }

  task_id_t id = sched_clone_user_task(parent->task_id, child->l2_table, child->pid, 0);
  if (id == NO_TASK) {
    goto process_clone_error;
  }
  child->task_id = id;
  LOG(LOG_PROC "Cloned process %d from parent %d, child task id %ld\n", child->pid, parent->pid, child->task_id);

  unlock_processes_ctx();

  return child->pid;

process_clone_error:
  unlock_processes_ctx();
  (void)process_destroy(child->pid);
  return -1;
}

int process_load_l2_table(pid_t pid) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }
  mmu_set_user_l2_table(process->l2_table);
  return 0;
}

int process_unload_l2_table(pid_t pid) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }
  mmu_set_user_l2_table(NULL);
  return 0;
}

int process_open_file(pid_t pid, const char* path, int flags, mode_t mode) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }

  VFSFileDescriptor* vfs_fd = vfs_open(path, flags, mode);
  if (vfs_fd == NULL) {
    return -1;
  }

  for (int i = 0; i < MAX_OPEN_FDS; i++) {
    if (process->open_fds[i].vfs_fd == NULL) {
      process->open_fds[i].fd = i;
      process->open_fds[i].vfs_fd = vfs_fd;
      return i;
    }
  }

  // No space for new fd
  vfs_close(vfs_fd);
  return -1;
}

ssize_t process_write_file(pid_t pid, int fd, const void* buffer, size_t size) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }

  if (fd < 0 || fd >= MAX_OPEN_FDS || process->open_fds[fd].vfs_fd == NULL) {
    return -1;
  }

  return vfs_write(process->open_fds[fd].vfs_fd, buffer, size);
}

ssize_t process_read_file(pid_t pid, int fd, void* buffer, size_t size) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }

  if (fd < 0 || fd >= MAX_OPEN_FDS || process->open_fds[fd].vfs_fd == NULL) {
    return -1;
  }

  return vfs_read(process->open_fds[fd].vfs_fd, buffer, size);
}

int process_close_file(pid_t pid, int fd) {
  Process* process = get_process_by_pid(pid);
  if (process == NULL) {
    return -1;
  }

  if (fd < 0 || fd >= MAX_OPEN_FDS || process->open_fds[fd].vfs_fd == NULL) {
    return -1;
  }

  int res = vfs_close(process->open_fds[fd].vfs_fd);
  process->open_fds[fd].vfs_fd = NULL;
  process->open_fds[fd].fd = 0;
  return res;
}