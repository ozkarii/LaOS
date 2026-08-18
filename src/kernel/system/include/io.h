#ifndef IO_H
#define IO_H

#define EOF (-1)

void k_putchar(const char c);
void k_puts(const char* s);

char k_getchar(void);

void k_printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

#endif // IO_H

