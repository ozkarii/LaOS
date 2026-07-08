#include <stdatomic.h>
#include <stddef.h>

#include "serial-buffer.h"
#include "sched.h"
#include "sem.h"

#define SERIAL_BUFFER_SIZE 256
#define SERIAL_BUFFER_MASK (SERIAL_BUFFER_SIZE - 1)

#if SERIAL_BUFFER_SIZE & SERIAL_BUFFER_MASK
#error "SERIAL_BUFFER_SIZE must be a power of 2"
#endif

// Lock free SPSC ring buffer for serial console input from UART
typedef struct SerialBuffer {
  _Atomic size_t head;
  _Atomic size_t tail;
  char buffer[SERIAL_BUFFER_SIZE];
  KSemaphore buffer_sem;  // Semaphore to signal when data is available
} SerialBuffer;

static SerialBuffer serial_buffer = {
  .head = ATOMIC_VAR_INIT(0),
  .tail = ATOMIC_VAR_INIT(0),
  .buffer = {0},
  .buffer_sem = K_SEM_INIT_IRQ_SAFE(0, SERIAL_BUFFER_SIZE)
};

int serial_buffer_put(char c) {
  size_t head = atomic_load_explicit(&serial_buffer.head, memory_order_relaxed);
  size_t tail = atomic_load_explicit(&serial_buffer.tail, memory_order_acquire);

  if ((head - tail) == SERIAL_BUFFER_SIZE) {
    return -1; // full
  }

  serial_buffer.buffer[head & SERIAL_BUFFER_MASK] = c;
  atomic_store_explicit(&serial_buffer.head, head + 1, memory_order_release);

  // Signal data available
  k_sem_try_post(&serial_buffer.buffer_sem);

  return 0;
}

char serial_buffer_get(void) {
  size_t tail = atomic_load_explicit(&serial_buffer.tail, memory_order_relaxed);
  size_t head = atomic_load_explicit(&serial_buffer.head, memory_order_acquire);

  while (tail == head) {
    // No data available, wait for data to be put into the buffer
    k_sem_wait(&serial_buffer.buffer_sem);
    tail = atomic_load_explicit(&serial_buffer.tail, memory_order_relaxed);
    head = atomic_load_explicit(&serial_buffer.head, memory_order_acquire);
  }

  char c = serial_buffer.buffer[tail & SERIAL_BUFFER_MASK];
  atomic_store_explicit(&serial_buffer.tail, tail + 1, memory_order_release);
  return c;
}
