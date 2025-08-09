#ifndef IO_H
#define IO_H

void k_putchar(const char c);
void k_puts(const char* s);

char k_getchar(void);
char* k_gets(char* s, int max_len);

void k_printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

#endif // IO_H

