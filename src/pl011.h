#ifndef PL011_H
#define PL011_H

#include <stdbool.h>
#include <stdint.h>

#define UARTDR 0x0

#define UARTFR 0x18
#define UARTFR_BUSY (1u << 3u)
#define UARTFR_TXFF (1u << 5u)
#define UARTFR_RXFF (1u << 6u)

#define UARTCR 0x30
#define UARTCR_UARTEN 1u
#define UARTCR_TXE (1u << 8u)
#define UARTCR_RXE (1u << 9u)

#define UARTLCR_H 0x2c
#define UARTLCR_H_FEN (1u << 4u)

#define UARTIMSC 0x38
#define UARTIMSC_RXIM (1u << 4u)
#define UARTIMSC_TXIM (1u << 5u)

#define UARTRIS 0x3C
#define UARTRIS_RXRIS (1u << 4u)
#define UARTRIS_TXRIS (1u << 5u)

#define UARTMIS  0x40

void pl011_enable(void);
int pl011_busy(void);
int pl011_rx_full(void);
void pl011_putc(const char c);
char pl011_getc(void);
void pl011_set_rx_irq(bool enable);
uint32_t pl011_get_raw_irq_status(void);
uint32_t pl011_get_masked_irq_status(void);
void pl011_print_info(void (*printf_func)(const char* format, ...));


#endif // PL011_H