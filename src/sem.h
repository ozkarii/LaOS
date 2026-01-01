#ifndef SEM_H
#define SEM_H

#include <stdint.h>
#include "sched.h"


typedef struct KSemaphore {
  _Atomic uint64_t value;
  uint64_t max_value;
  uint8_t wait_queue[MAX_TASKS];
  uint32_t wait_count;
} KSemaphore;


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value);
int k_sem_wait(KSemaphore* sem);
int k_sem_post(KSemaphore* sem);
int k_sem_destroy(KSemaphore* sem);


#endif /* SEM_H */