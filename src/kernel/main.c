/**
 * @file main.c
 * @brief Entry point to the OS kernel C code
 */

#include <stdint.h>
#include "stdio.h"
#include "string.h"

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

#define INIT_BIN_LOAD_ADDR 0x70000000UL 
#define INIT_BIN_PATH      "/sbin/init"

const char* INITIAL_RAMFS_DIRECTORIES[] = {
  "/sbin"
};


void console_loop_task(void) {
  console_loop("#");
}

void task_sleep_demo(void) {
  int counter = 0;
  while (1) {
    k_printf("task_sleep_demo: sleeping for 5 seconds, counter: %d\r\n", counter);
    sched_sleep(1000000);
    k_printf("task_sleep_demo: woke up, counter: %d\r\n", counter);
    counter++;
  }
}


static int copy_init_bin_from_memory_to_file(VFSFileDescriptor* fd, uint8_t* src) {
  const size_t init_bin_size = (1024 * 8);

  size_t bytes_written = vfs_write(fd, src, init_bin_size);

  if (bytes_written != init_bin_size) {
    k_printf(LOG_FS "Failed to write to init binary file in RamFS\n");
    k_printf(LOG_FS "Wrote 0x%lx bytes\n", bytes_written);
    return -1;
  }

  return 0;
}

int setup_ramfs(void) {
  // Allocate 64 MB for ramfs
  static uint8_t ramfs_buffer[0x4000000];

  const size_t ramfs_size = ramfs_get_size();
  k_printf(LOG_FS "RamFS size: 0x%lx\n", ramfs_size);

  if (ramfs_size > sizeof(ramfs_buffer)) {
    k_printf(LOG_FS  "Size of RamFS is too big for ramfs_buffer!\n");
    return -1;
  }

  void* ramfs = ramfs_init((void*)ramfs_buffer, sizeof(ramfs_buffer));
  if (ramfs == NULL) {
    k_printf(LOG_FS "Failed to initialize RamFS\n");
    return -1;
  }

  VFSInterface* ramfs_if = ramfs_get_vfs_interface();
  vfs_mount("/", ramfs_if, ramfs);
  k_printf(LOG_FS "Mounted RamFS at /\n");

  // Create initial directories
  const size_t num_dirs = 
    sizeof(INITIAL_RAMFS_DIRECTORIES) / sizeof(INITIAL_RAMFS_DIRECTORIES[0]);

  for (unsigned i = 0; i < num_dirs; i++) {
    int ret = vfs_mkdir(INITIAL_RAMFS_DIRECTORIES[i]);
    if (ret != 0) {
      k_printf(LOG_FS "Failed to create initial RamFS directory %s\n", INITIAL_RAMFS_DIRECTORIES[i]);
      return -1;
    }
  }
  k_printf(LOG_FS "Created initial RamFS directories\n");


  VFSFileDescriptor* init_process_fd = vfs_open(INIT_BIN_PATH, MODE_CREATE | MODE_WRITE | MODE_READ);
  if (init_process_fd == NULL) {
    k_printf(LOG_FS "Failed to create file %s for init process\n", INIT_BIN_PATH);
    return -1;
  }
  k_printf(LOG_FS "Created init process file %s in RamFS\n", INIT_BIN_PATH);

  int ret = copy_init_bin_from_memory_to_file(init_process_fd, (uint8_t*)INIT_BIN_LOAD_ADDR);
  if (ret != 0) {
    k_printf(LOG_FS "Failed to copy init process binary from 0x%lx to RamFS\n", INIT_BIN_LOAD_ADDR);
    return -1;
  }

  vfs_close(init_process_fd);

  return 0;
}

static _Atomic bool primary_cpu_started = false;

int c_entry() {
  mmu_init();
  
  pl011_enable();
  pl011_set_rx_irq(true);

  gicd_enable_irq(UART_IRQ);
  gicd_enable_irq(EL1_PHY_TIM_IRQ);

  gicd_set_irq_priority(UART_IRQ, 1);
  gicd_set_irq_priority(EL1_PHY_TIM_IRQ, 0);

  gicd_set_irq_cpu(UART_IRQ, 0);

  gicc_set_priority_mask(0xFF, GET_CPU_ID());
  
  gicd_enable();
  gicc_enable(GET_CPU_ID());

  UNMASK_ALL_INTERRUPTS();

  sched_init(100000, gicc_end_irq);

  if (setup_ramfs() != 0) {
    k_printf(LOG_KERNEL "Failed to setup RamFS, cannot proceed\n");
    while (1);
  }

  pid_t init_process_pid = process_create_init_process();
  if (init_process_pid < 0) {
    k_printf(LOG_KERNEL "Failed to create init process, cannot proceed\n");
    while (1);
  } else {
    k_printf(LOG_KERNEL "Created init process with PID %d\n", init_process_pid);
  }

  k_printf(LOG_KERNEL "Primary CPU0 started\r\n");
  primary_cpu_started = true;

  sched_start();

  return 0;
}

int c_entry_secondary_core(void) {
  while (!primary_cpu_started) {
    // Wait for primary CPU to finish init
  }

  uint32_t cpu_id = GET_CPU_ID();
  k_printf(LOG_KERNEL "Secondary CPU%u starting up...\r\n", cpu_id);

  mmu_init();

  gicd_enable_irq(EL1_PHY_TIM_IRQ);
  gicc_set_priority_mask(0xFF, cpu_id);
  gicc_enable(cpu_id);

  switch (cpu_id) {
  case 1:
    sched_create_kernel_task(console_loop_task);
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    k_printf(LOG_KERNEL "Dubious CPU%u entering idle loop...\r\n", cpu_id);
    while (1) {
      WAIT_FOR_INTERRUPT();
    }
  }

  sched_start();

  return 0;
}
