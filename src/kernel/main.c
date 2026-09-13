/**
 * @file main.c
 * @brief Entry point to the OS kernel C code
 */

#include <stdint.h>
#include "stdio.h"
#include "string.h"
#include "fcntl.h"

#include "io.h"
#include "console.h"
#include "gic.h"
#include "armv8-a.h"
#include "platform.h"
#include "pl011.h"
#include "sched.h"
#include "sem.h"
#include "mmu.h"
#include "vfs.h"
#include "ramfs.h"
#include "log.h"
#include "process.h"
#include "ramdisk.h"

#define RAMDISK_ADDR 0x70000000UL

const char* INITIAL_RAMFS_DIRECTORIES[] = {
  "/sbin",
  "/dev",
  "/bin"
};


void console_loop_task(void* arg) {
  (void)arg;

  console_loop("#");
}


static int copy_bin_from_memory_to_file(VFSFileDescriptor* fd, uint8_t* src, size_t size) {
  size_t bytes_written = vfs_write(fd, src, size);

  if (bytes_written != size) {
    k_printf(LOG_MAIN "Failed to write to binary file in RamFS\n");
    k_printf(LOG_MAIN "Wrote 0x%lx bytes\n", bytes_written);
    return -1;
  }

  return 0;
}

static int ensure_directory_exists(const char* path) {
  VFSStat stat;
  if (vfs_stat(path, &stat) == 0) {
    return stat.is_directory ? 0 : -1;
  }

  return vfs_mkdir(path, 0);
}

static int ensure_parent_directories(const char* path) {
  size_t path_len = strlen(path);
  if (path_len == 0 || path[0] != '/') {
    return -1;
  }

  char dir_path[PATH_NAME_MAX];
  size_t dir_len = 0;

  for (size_t i = 1; i < path_len; i++) {
    if (path[i] != '/') {
      continue;
    }

    if (i >= PATH_NAME_MAX) {
      return -1;
    }

    memcpy(dir_path, path, i);
    dir_path[i] = '\0';
    dir_len = i;

    if (dir_len > 0 && ensure_directory_exists(dir_path) != 0) {
      return -1;
    }
  }

  return 0;
}

int setup_ramfs(void) {
  // Allocate 64 MB for ramfs
  static uint8_t ramfs_buffer[0x4000000];

  const size_t ramfs_size = ramfs_get_size();
  k_printf(LOG_MAIN "RamFS size: 0x%lx\n", ramfs_size);

  if (ramfs_size > sizeof(ramfs_buffer)) {
    k_printf(LOG_MAIN  "Size of RamFS is too big for ramfs_buffer!\n");
    return -1;
  }

  void* ramfs = ramfs_init((void*)ramfs_buffer, sizeof(ramfs_buffer));
  if (ramfs == NULL) {
    k_printf(LOG_MAIN "Failed to initialize RamFS\n");
    return -1;
  }

  VFSInterface* ramfs_if = ramfs_get_vfs_interface();
  if (vfs_mount("/", ramfs_if, ramfs) != 0) {
    k_printf(LOG_MAIN "Failed to mount RamFS at /\n");
    return -1;
  }
  k_printf(LOG_MAIN "Mounted RamFS at /\n");

  const void* ramdisk_image = (const void*)RAMDISK_ADDR;
  if (!ramdisk_is_valid(ramdisk_image)) {
    k_printf(LOG_MAIN "Invalid ramdisk image at 0x%lx\n", RAMDISK_ADDR);
    return -1;
  }
  k_printf(LOG_MAIN "Valid ramdisk image found at 0x%lx\n", RAMDISK_ADDR);
  
  int num_files = ramdisk_get_file_count(ramdisk_image);
  if (num_files > ramfs_get_max_files()) {
    k_printf(LOG_MAIN "Ramdisk has too many files for RamFS\n");
    return -1;
  }
  k_printf(LOG_MAIN "Loading ramdisk from 0x%lx\n", RAMDISK_ADDR);

  for (int i = 0; i < num_files; i++) {
    const RamdiskEntry* entry = ramdisk_get_entry(ramdisk_image, i);
    if (entry == NULL) {
      k_printf(LOG_MAIN "Failed to get ramdisk entry %d\n", i);
      return -1;
    }

    const uint8_t* data = ramdisk_get_data(ramdisk_image, entry);
    if (data == NULL) {
      k_printf(LOG_MAIN "Failed to get ramdisk data for entry %d\n", i);
      return -1;
    }

    if (ensure_parent_directories(entry->path) != 0) {
      k_printf(LOG_MAIN "Failed to create parent directories for %s\n", entry->path);
      return -1;
    }
    
    VFSFileDescriptor* fd = vfs_open(entry->path, O_CREAT | O_WRONLY, 0);
    if (fd == NULL) {
      k_printf(LOG_MAIN "Failed to create file %s in RamFS\n", entry->path);
      return -1;
    }
    k_printf(LOG_MAIN "Creating file %s in RamFS with size 0x%x\n", entry->path, entry->size);

    if (copy_bin_from_memory_to_file(fd, (uint8_t*)data, entry->size) != 0) {
      vfs_close(fd);
      return -1;
    }

    vfs_close(fd);
  }

  // Placeholder console file for console output, since we don't have a real device driver yet
  if (ensure_directory_exists("/dev") != 0) {
    k_printf(LOG_MAIN "Failed to create /dev directory in RamFS\n");
    return -1;
  }
  VFSFileDescriptor* console_fd = vfs_open("/dev/console", O_CREAT, 0);
  if (console_fd == NULL) {
    k_printf(LOG_MAIN "Failed to create /dev/console in RamFS\n");
    return -1;
  }
  vfs_close(console_fd);

  return 0;
}

static _Atomic bool primary_cpu_started = false;

int c_entry() {
  mmu_init();
  
  pl011_enable();
  pl011_set_rx_irq(true);

  k_printf(LOG_MAIN "PL011 UART initialized\n");

  gicd_enable_irq(UART_IRQ);
  gicd_enable_irq(EL0_PHY_TIM_IRQ);

  gicd_set_irq_priority(UART_IRQ, 1);
  gicd_set_irq_priority(EL0_PHY_TIM_IRQ, 0);

  gicd_set_irq_cpu(UART_IRQ, 0);

  gicc_set_priority_mask(0xFF, GET_CPU_ID());
  
  gicd_enable();
  gicc_enable(GET_CPU_ID());

  UNMASK_ALL_INTERRUPTS();

  k_printf(LOG_MAIN "PL011 UART IRQ enabled\n");
  k_printf(LOG_MAIN "cntpct_el0 IRQ enabled\n");

  sched_init(gicc_end_irq);

  static VFS vfs;
  if (vfs_init(&vfs) != 0) {
    k_printf(LOG_MAIN "Failed to initialize VFS, cannot proceed\n");
    while (1);
  }
  k_printf(LOG_MAIN "VFS initialized\n");

  if (setup_ramfs() != 0) {
    k_printf(LOG_MAIN "Failed to setup RamFS, cannot proceed\n");
    while (1);
  }

  pid_t init_process_pid = process_create_init_process();
  if (init_process_pid < 0) {
    k_printf(LOG_MAIN "Failed to create init process, cannot proceed\n");
    while (1);
  } else {
    k_printf(LOG_MAIN "Created init process with PID %d\n", init_process_pid);
  }

  k_printf(LOG_MAIN "Primary CPU0 started\r\n");
  primary_cpu_started = true;

  sched_start();

  return 0;
}

int c_entry_secondary_core(void) {
  while (!primary_cpu_started) {
    // Wait for primary CPU to finish init
  }

  uint32_t cpu_id = GET_CPU_ID();
  k_printf(LOG_MAIN "CPU%u starting up...\r\n", cpu_id);

  mmu_init();

  k_printf(LOG_MAIN "CPU%u MMU initialized\n", cpu_id);

  gicd_enable_irq(EL0_PHY_TIM_IRQ);
  gicc_set_priority_mask(0xFF, cpu_id);
  gicc_enable(cpu_id);

  k_printf(LOG_MAIN "cntpct_el0 IRQ enabled on CPU%u\n", cpu_id);

  switch (cpu_id) {
  case 1:
    sched_create_kernel_task(console_loop_task, NULL);
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    k_printf(LOG_MAIN "Dubious CPU%u entering idle loop...\r\n", cpu_id);
    while (1) {
      WAIT_FOR_INTERRUPT();
    }
  }

  sched_start();

  return 0;
}
