#ifndef SEM_H
#define SEM_H

#include <stdint.h>
#include "sched.h"
#include "spinlock.h"


typedef struct KSemaphore {
  uint64_t value;
  uint64_t max_value;
  uint8_t wait_queue[MAX_TASKS];
  uint32_t wait_count;
  uint32_t queue_front;  // Index of oldest item
  uint32_t queue_rear;   // Index where next item will be added
  Spinlock lock;
} KSemaphore;


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value);
int k_sem_wait(KSemaphore* sem);
int k_sem_post(KSemaphore* sem);
uint64_t k_sem_get_value(KSemaphore* sem);

#endif /* SEM_H */