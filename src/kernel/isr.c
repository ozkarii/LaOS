/**
 * @file isr.c
 * @brief Interrupt Service Routines and Exception Handlers
 */

#include "gic.h"
#include "io.h"
#include "pl011.h"
#include "platform.h"
#include "armv8-a.h"
#include "sched.h"
#include "io-buffer.h"


void sync_exception_handler(void) {
  k_puts("sync_exception_handler\r\n");
  cpu_dump_registers(k_printf);
  while(1);
}

void irq_exception_handler(uintptr_t sp_after_ctx_save) {
  uint32_t cpu_id = GET_CPU_ID();
  uint32_t int_id = gicc_get_intid_and_ack(cpu_id);

  switch (int_id) {
  case UART_IRQ:
    serial_buffer_putc(pl011_getc);
    break;
  case EL1_PHY_TIM_IRQ:
    sched_timer_irq_handler(EL1_PHY_TIM_IRQ, cpu_id, sp_after_ctx_save);
    break;
  default:
    k_printf("Got unknown IRQ with ID %x on CPU%u\n", int_id, cpu_id);
    break;
  }
  gicc_end_irq(int_id, cpu_id);
}

void fiq_exception_handler(void) {
  k_puts("fiq_exception_handler\r\n");
  while(1);
}

void serror_exception_handler(void) {
  k_puts("serror_exception_handler\r\n");
  cpu_dump_registers(k_printf);
  while(1);
}