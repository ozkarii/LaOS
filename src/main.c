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

int c_entry() {
  pl011_enable();
  pl011_set_rx_irq(true);

  gicd_enable_irq(UART_IRQ);
  gicd_set_irq_priority(UART_IRQ, 0);
  gicd_set_irq_cpu(UART_IRQ, 0);
  gicc_set_priority_mask(0xFF);
  
  gicd_enable();
  gicc_enable();

  ENABLE_ALL_INTERRUPTS();

  // startup_logs();

  console_loop("#");
  
  return 0;
} 
