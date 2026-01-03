/**
 * @file main.c
 * @brief Entry point to the Laudes operating system.
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

void console_loop_task(void) {
  console_loop("#");
}

void task_0(void) {
  int counter = 0;
  while (1) {
    k_printf("task_0 running, counter: %d\r\n", counter);
    for (volatile int i = 0; i < 10000000; i++);
    counter++;
  }
}

void task_1(void) {
  int counter = 0;
  while (1) {
    k_printf("task_1 running, counter: %d\r\n", counter);
    for (volatile int i = 0; i < 10000000; i++);
    counter += 2;
  }
}

void task_2(void) {
  int counter = 0;
  while (1) {
    k_printf("task_2 running, counter: %d\r\n", counter);
    for (volatile int i = 0; i < 80000000; i++);
    counter += 4;
    sched_yield();
  }
}

void task_3(void) {
  int counter = 0;
  while (1) {
    k_printf("task_3 running, counter: %d\r\n", counter);
    for (volatile int i = 0; i < 80000000; i++);
    counter += 8;
    sched_yield();
  }
}


void task_sleep_demo(void) {
  int counter = 0;
  while (1) {
    k_printf("task_sleep_demo: sleeping for 5 seconds, counter: %d\r\n", counter);
    sched_sleep(5000000);  // Sleep for 5 seconds
    k_printf("task_sleep_demo: woke up, counter: %d\r\n", counter);
    counter++;
  }
}

KSemaphore test_sem;

void sem_wait_task(void) {
  while (1) {
    for (int i = 0; i < 10; i++) {
      k_sem_wait(&test_sem);
      k_printf("got semaphore\r\n");
    }
    k_printf("waiting one more\r\n");
    k_sem_wait(&test_sem);
    k_printf("k_sem_wait unblocked\r\n");
  }
}

void sem_post_task(void) {
  k_sem_init(&test_sem, 0, 10);
  while (1) {
    for (int i = 0; i < 10; i++) {
      k_sem_post(&test_sem);
      k_printf("posted to semaphore\r\n");
    }
    k_printf("posting one more\r\n");
    k_sem_post(&test_sem);
    k_printf("k_sem_post unblocked\r\n");
  }
}

int c_entry() {
  pl011_enable();
  pl011_set_rx_irq(true);

  gicd_enable_irq(UART_IRQ);
  gicd_enable_irq(EL1_PHY_TIM_IRQ);

  gicd_set_irq_priority(UART_IRQ, 1);
  gicd_set_irq_priority(EL1_PHY_TIM_IRQ, 0);

  gicd_set_irq_cpu(UART_IRQ, 0);
  gicd_set_irq_cpu(EL1_PHY_TIM_IRQ, 0);

  gicc_set_priority_mask(0xFF);
  
  gicd_enable();
  gicc_enable();

  ENABLE_ALL_INTERRUPTS();

  startup_logs();

  sched_init(1000000, gicc_end_irq);
  sched_create_task(task_0);
  sched_create_task(task_1);
  sched_create_task(task_2);
  sched_create_task(task_3);
  sched_create_task(task_sleep_demo);
  sched_create_task(console_loop_task);
  sched_create_task(sem_post_task);
  sched_create_task(sem_wait_task);

  sched_start();

  return 0;
} 