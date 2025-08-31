/**
 * @file isr.c
 * @brief Interrupt Service Routines and Exception Handlers
 */

#include "gic.h"
#include "io.h"
#include "pl011.h"
#include "platform.h"

void sync_exception_handler(void) {
    k_puts("sync_exception_handler\r\n");
    while(1);
}

void irq_exception_handler(void) {
    uint32_t intid = gicc_get_intid_and_ack();
    k_puts("irq_exception_handler\r\n");
    if (intid == UART_IRQ) pl011_getc();
    gicc_end_irq(intid);
}

void fiq_exception_handler(void) {
    k_puts("fiq_exception_handler\r\n");
    while(1);
}

void serror_exception_handler(void) {
    k_puts("serror_exception_handler\r\n");
    while(1);
}