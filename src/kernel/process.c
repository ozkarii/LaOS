#include "string.h"

#include "io.h"
#include "process.h"
#include "vfs.h"
#include "log.h"
#include "sched.h"
#include "memory.h"
#include "mmu.h"
#include "armv8-a.h"


typedef struct FileDescriptor {
  fd_t fd;
  VFSFileDescriptor* vfs_fd;
} FileDescriptor;

typedef struct Process {
  pid_t pid;
  task_id_t task_id;
  bool allocated;
  FileDescriptor open_fds[MAX_OPEN_FDS];
  uint64_t* l2_table;
  void* code_block_phys_addr;
  void* data_block_phys_addr;
} Process;


static Process processes[MAX_PROCESSES] = {0};
static _Atomic pid_t next_pid = 1;


static Process* create_process(void) {
  Process *p = NULL;

  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!processes[i].allocated) {
      p = &processes[i];
      break;
    }
  }
  if (p == NULL) {
    return NULL;
  }

  p->allocated = true;
  p->l2_table = mmu_create_user_l2_table();

  if (p->l2_table == NULL) {
    goto mmu_create_user_l2_table_fail;
  }

  p->code_block_phys_addr = allocate_user_memory_block(p->l2_table, true);
  if (p->code_block_phys_addr == NULL) {
    goto user_code_block_alloc_fail;
  }

  p->data_block_phys_addr = allocate_user_memory_block(p->l2_table, false);
  if (p->data_block_phys_addr == NULL) {
    goto user_data_block_alloc_fail;
  }

  p->task_id = sched_create_user_task((uintptr_t)0x200000, p->l2_table, GET_CPU_ID());

  if (p->task_id == NO_TASK) {
    goto sched_create_user_task_fail;
  }

  p->pid = next_pid++;

  return p;


sched_create_user_task_fail:
  free_user_memory_block(p->l2_table, p->code_block_phys_addr);
  p->code_block_phys_addr = NULL;
user_data_block_alloc_fail:
  free_user_memory_block(p->l2_table, p->data_block_phys_addr);
  p->data_block_phys_addr = NULL;
user_code_block_alloc_fail:
  mmu_free_user_l2_table(p->l2_table);
  p->l2_table = NULL;
mmu_create_user_l2_table_fail:
  p->allocated = false;
  
  return NULL;
}

void process_destroy(pid_t pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!processes[i].allocated || processes[i].pid != pid) {
      continue;
    }
    for (int j = 0; j < MAX_OPEN_FDS; j++) {
      if (processes[i].open_fds[j].vfs_fd != NULL) {
        if (vfs_close(processes[i].open_fds[j].vfs_fd) != 0) {
          k_printf(LOG_KERNEL "Failed to close fd %d for process %d during destruction\n",
                   processes[i].open_fds[j].fd, pid);
        }
      }
    }

    mmu_free_user_l2_table(processes[i].l2_table);

    memset(processes[i].open_fds, 0, sizeof(FileDescriptor));
    memset(&processes[i], 0, sizeof(Process));
    return;
  }
}

pid_t process_create_init_process(void) {
  if (next_pid != 1) {
    return -1;
  }

  Process* p = create_process();

  if (p == NULL) {
    return -1;
  }

  VFSFileDescriptor* init_bin_fd = vfs_open("/sbin/init", MODE_READ);
  if (init_bin_fd == NULL) {
    process_destroy(p->pid);
    return -1;
  }

  p->open_fds[0].fd = 0;
  p->open_fds[0].vfs_fd = init_bin_fd;

  VFSStat stat;
  if (vfs_stat("/sbin/init", &stat) != 0) {
    vfs_close(init_bin_fd);
    process_destroy(p->pid);
    return -1;
  }

  if (vfs_read(init_bin_fd, p->code_block_phys_addr, stat.size) != stat.size) {
    vfs_close(init_bin_fd);
    process_destroy(p->pid);
    return -1;
  }

  return p->pid;
}

pid_t process_clone(pid_t parent_pid) {
  Process* parent = NULL;

  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (processes[i].allocated && processes[i].pid == parent_pid) {
      parent = &processes[i];
      break;
    }
  }

  if (parent == NULL) {
    return -1;
  }

  Process* child = create_process();

  if (child == NULL) {
    return -1;
  }

  // Copy open fds
  for (int i = 0; i < MAX_OPEN_FDS; i++) {
    if (parent->open_fds[i].vfs_fd != NULL) {
      child->open_fds[i] = parent->open_fds[i];
    }
  }

  clone_process_page_table(child->l2_table, parent->l2_table);

  return child->pid;
}

