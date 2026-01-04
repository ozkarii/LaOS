#ifndef SEM_H
#define SEM_H

#include <stdint.h>
#include "sched.h"
#include "spinlock.h"

typedef struct TaskQueue {
  task_id_t tasks[MAX_TASKS];
  uint32_t front;
  uint32_t rear;
  uint32_t count;
} TaskQueue;


typedef struct KSemaphore {
  uint64_t value;
  uint64_t max_value;
  TaskQueue wait_queue;
  TaskQueue post_queue;
  Spinlock lock;
} KSemaphore;


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value);
int k_sem_wait(KSemaphore* sem);
int k_sem_post(KSemaphore* sem);

// IRQ safe:
int k_sem_try_wait(KSemaphore* sem);
int k_sem_try_post(KSemaphore* sem);
uint64_t k_sem_get_value(KSemaphore* sem);

#endif /* SEM_H */