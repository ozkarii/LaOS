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

void ramfs_test_task(void) {
  VFSFileDescriptor* fd = vfs_open("/testfile.txt", MODE_CREATE | MODE_WRITE);
  if (fd == NULL) {
    k_printf("ramfs_test_task: failed to open file for writing\r\n");
    return;
  }

  const char* message = "Hello, RamFS!\n";
  vfs_write(fd, message, strlen(message));
  vfs_close(fd);

  fd = vfs_open("/testfile.txt", MODE_READ);
  if (fd == NULL) {
    k_printf("ramfs_test_task: failed to open file for reading\r\n");
    return;
  }

  char buffer[64];
  int bytes_read = vfs_read(fd, buffer, sizeof(buffer) - 1);
  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';  // Null-terminate the string
    k_printf("ramfs_test_task: read from file: %s", buffer);
  } else {
    k_printf("ramfs_test_task: failed to read from file\r\n");
  }

  vfs_close(fd);

  while (1) {
    sched_sleep(1000000000);
  }
}

static _Atomic bool primary_cpu_started = false;

// Allocate plenty (64 MB) for ramfs
uint8_t ramfs_buffer[0x4000000];

int c_entry() {
  mmu_init(true);
  
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

  ENABLE_ALL_INTERRUPTS();

  void* ramfs = ramfs_init((void*)ramfs_buffer, sizeof(ramfs_buffer));
  if (ramfs == NULL) {
    k_printf("Failed to initialize ramfs\n");
    while(1);
  }

  VFSInterface* ramfs_if = ramfs_get_vfs_interface();
  vfs_mount("/", ramfs_if, ramfs);
  k_printf("Mounted RamFS at /\r\n");

  sched_init(100000, gicc_end_irq);

  sched_create_task(ramfs_test_task);

  k_printf("Primary CPU0 starting up...\r\n");
  primary_cpu_started = true;

  sched_start();

  return 0;
}

int c_entry_secondary_core(void) {
  while (!primary_cpu_started) {
    // Wait for primary CPU to finish init
  }

  uint32_t cpu_id = GET_CPU_ID();
  k_printf("Secondary CPU%u starting up...\r\n", cpu_id);

  mmu_init(false);

  gicd_enable_irq(EL1_PHY_TIM_IRQ);
  gicc_set_priority_mask(0xFF, cpu_id);
  gicc_enable(cpu_id);

  switch (cpu_id) {
  case 1:
    sched_create_task(console_loop_task);
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    k_printf("Dubious CPU%u entering idle loop...\r\n", cpu_id);
    WAIT_FOR_INTERRUPT();
    break;
  }

  sched_start();

  return 0;
}
