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

void console_loop_task(void) {
  console_loop("#");
}

void hello_laudes_task(void) {
  while (1) {
    k_printf("lau timval: %lx\r\n", GET_PHYS_TIMER_VALUE());
    for (volatile int i = 0; i < 80000000; i++);
  }
}

void hello_world_task(void) {
  while (1) {
    k_printf("world timval: %lx\r\n", GET_PHYS_TIMER_VALUE());
    for (volatile int i = 0; i < 80000000; i++);
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

  sched_init(1000000);
  sched_create_task(hello_laudes_task);
  sched_create_task(hello_world_task);
  sched_start();

  return 0;
} 
