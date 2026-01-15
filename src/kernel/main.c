/**
 * @file main.c
 * @brief C entry point to OS kernel
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

void console_loop_task(void) {
  console_loop("#");
}

void task_sleep_demo(void) {
  int counter = 0;
  while (1) {
    k_printf("task_sleep_demo: sleeping for 5 seconds, counter: %d\r\n", counter);
    sched_sleep(5000000);
    k_printf("task_sleep_demo: woke up, counter: %d\r\n", counter);
    counter++;
  }
}

KSemaphore test_sem;

void sem_wait_task(void) {
  while (1) {
    for (int i = 0; i < 10; i++) {
      k_sem_wait(&test_sem);
    }
    k_sem_wait(&test_sem);
  }
}

void sem_post_task(void) {
  k_sem_init(&test_sem, 0, 10);
  while (1) {
    for (int i = 0; i < 10; i++) {
      k_sem_post(&test_sem);
    }
    k_sem_post(&test_sem);
  }
}

static _Atomic bool primary_cpu_started = false;

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

  startup_logs();

  sched_init(100000, gicc_end_irq);

  sched_create_task(task_sleep_demo);

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
    sched_create_task(sem_post_task);
    break;
  case 2:
    sched_create_task(sem_wait_task);
    break;
  case 3:
    sched_create_task(console_loop_task);
    break;
  default:
    k_printf("Dubious CPU%u entering idle loop...\r\n", cpu_id);
    WAIT_FOR_INTERRUPT();
    break;
  }

  sched_start();

  return 0;
}
