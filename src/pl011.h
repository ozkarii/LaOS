#ifndef PL011_H
#define PL011_H

#define UARTDR 0x0

#define UARTFR 0x18
#define UARTFR_BUSY (1 << 3u)
#define UARTFR_TXFF (1 << 5u)

#define UARTCR 0x30
#define UARTCR_TXE (1 << 8u)


int pl011_busy(void);
void pl011_putc(const char c);

#endif // PL011_H