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
  void* code_block_pa;
  void* data_block_pa;
} Process;

typedef struct ProcessesContext {
  Process processes[MAX_PROCESSES];
  pid_t pid_counter;
  Spinlock lock;
} ProcessesContext;


static ProcessesContext ctx;


static Process* create_process(void) {
  Process *p = NULL;

  spinlock_acquire(&ctx.lock);

  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!ctx.processes[i].allocated) {
      p = &ctx.processes[i];
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

  p->code_block_pa = allocate_user_memory_block(p->l2_table, true);
  if (p->code_block_pa == NULL) {
    goto user_code_block_alloc_fail;
  }

  p->data_block_pa = allocate_user_memory_block(p->l2_table, false);
  if (p->data_block_pa == NULL) {
    goto user_data_block_alloc_fail;
  }

  p->pid = ctx.pid_counter++;

  spinlock_release(&ctx.lock);

  return p;

user_data_block_alloc_fail:
  free_user_memory_block(p->l2_table, p->code_block_pa);
  p->code_block_pa = NULL;
user_code_block_alloc_fail:
  mmu_free_user_l2_table(p->l2_table);
  p->l2_table = NULL;
mmu_create_user_l2_table_fail:
  p->allocated = false;
no_space:
  spinlock_release(&ctx.lock);
  return NULL;
}

void process_destroy(pid_t pid) {
  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (!ctx.processes[i].allocated || ctx.processes[i].pid != pid) {
      continue;
    }
    spinlock_acquire(&ctx.lock);
    for (int j = 0; j < MAX_OPEN_FDS; j++) {
      if (ctx.processes[i].open_fds[j].vfs_fd != NULL) {
        if (vfs_close(ctx.processes[i].open_fds[j].vfs_fd) != 0) {
          k_printf(LOG_KERNEL "Failed to close fd %d for process %d during destruction\n",
                   ctx.processes[i].open_fds[j].fd, pid);
        }
      }
    }
    mmu_free_user_l2_table(ctx.processes[i].l2_table);

    memset(ctx.processes[i].open_fds, 0, sizeof(ctx.processes[i].open_fds));
    memset(&ctx.processes[i], 0, sizeof(Process));

    spinlock_release(&ctx.lock);
    return;
  }
}

pid_t process_create_init_process(void) {
  if (ctx.pid_counter != 0) {
    return -1;
  }

  Process* p = create_process();

  if (p == NULL) {
    return -1;
  }

  VFSFileDescriptor* init_bin_fd = vfs_open("/sbin/init", MODE_READ);
  if (init_bin_fd == NULL) {
    goto destroy_process;
  }

  p->open_fds[0].fd = 0;
  p->open_fds[0].vfs_fd = init_bin_fd;

  VFSStat stat;
  if (vfs_stat("/sbin/init", &stat) != 0) {
    goto close_init_bin_fd;
  }

  void *tmp_elf = k_malloc(stat.size);
  if (tmp_elf == NULL) {
    goto close_init_bin_fd;
  }

  if (vfs_read(init_bin_fd, tmp_elf, stat.size) != stat.size) {
    goto free_tmp_elf;
  }

  uint64_t entry_offset = 0;
  if (elf_load_from_memory(tmp_elf, stat.size, p->code_block_pa,
                           p->data_block_pa, &entry_offset) != 0) {
    goto free_tmp_elf;
  }

  uintptr_t entry_va = (TEXT_SECTION_VA + entry_offset);
  sched_create_user_task(entry_va, p->l2_table, GET_CPU_ID(), STACK_TOP_VA);

  vfs_close(init_bin_fd);

  return p->pid;

free_tmp_elf:
  k_free(tmp_elf);
close_init_bin_fd:
  vfs_close(init_bin_fd);
destroy_process:
  process_destroy(p->pid);
  return -1;
}

pid_t process_clone(pid_t parent_pid) {
  Process* parent = NULL;


  for (int i = 0; i < MAX_PROCESSES; i++) {
    if (ctx.processes[i].allocated && ctx.processes[i].pid == parent_pid) {
      parent = &ctx.processes[i];
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

