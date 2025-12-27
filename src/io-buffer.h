#ifndef IO_BUFFER_H
#define IO_BUFFER_H
 
#include <stddef.h>

void serial_buffer_putc(char (*getc_func)(void));
char serial_buffer_getc(void);

 
#endif /* IO_BUFFER_H */