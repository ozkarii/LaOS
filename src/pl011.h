#ifndef PL011_H
#define PL011_H

#include "stdint.h"

#define UARTDR 0x0

#define UARTFR 0x18
#define UARTFR_BUSY (1u << 3u)
#define UARTFR_TXFF (1u << 5u)
#define UARTFR_RXFF (1u << 6u)

#define UARTCR 0x30
#define UARTCR_TXE (1u << 8u)

#define UARTLCR_H 0x2c
#define UARTLCR_H_FEN (1u << 4u)

int pl011_busy(void);
int pl011_rx_full(void);
void pl011_putc(const char c);
char pl011_getc(void);


#endif // PL011_H