#ifndef SERIAL_BUFFER_H
#define SERIAL_BUFFER_H
 
#include <stddef.h>

int serial_buffer_put(char c);
char serial_buffer_get(void);

 
#endif /* SERIAL_BUFFER_H */