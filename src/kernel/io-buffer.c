#include "io-buffer.h"
#include "io.h"

#define SERIAL_BUFFER_SIZE 256

typedef struct SerialBuffer {
  size_t head;
  size_t tail;
  size_t count;
  char buffer[SERIAL_BUFFER_SIZE];
} SerialBuffer;

SerialBuffer serial_buffer = {0, 0, 0, {0}};

void serial_buffer_putc(char (*getc_func)(void)) {
  serial_buffer.buffer[serial_buffer.head] = getc_func();
  serial_buffer.head = (serial_buffer.head + 1) % SERIAL_BUFFER_SIZE;
  if (serial_buffer.count < SERIAL_BUFFER_SIZE) {
    serial_buffer.count++;
  } else {
    serial_buffer.tail = (serial_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
  }
}

char serial_buffer_getc(void) {
  if (serial_buffer.count == 0) {
    return EOF; // Buffer is empty
  }
  char c = serial_buffer.buffer[serial_buffer.tail];
  serial_buffer.tail = (serial_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
  serial_buffer.count--;
  return c;
}