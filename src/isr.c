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

void sync_exception_handler(void) {
    k_puts("sync_exception_handler\r\n");
    cpu_dump_registers(k_printf);
    while(1);
}

void irq_exception_handler(void) {
    uint32_t intid = gicc_get_intid_and_ack();
    switch (intid) {
    case UART_IRQ:
        pl011_getc();
        break;
    case EL1_PHY_TIM_IRQ:
        k_printf("Got EL1_PHY_TIM_IRQ\n");
        sched_timer_irq_handler(EL1_PHY_TIM_IRQ);
        break;
    default:
        k_printf("Got unknown IRQ with ID %x\n", intid);
        break;
    }
    gicc_end_irq(intid);
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