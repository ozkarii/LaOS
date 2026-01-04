#include <stddef.h>
#include "string.h"
#include "sem.h"


static int push_task_to_queue(TaskQueue* queue, task_id_t task_id) {
  if (queue->count >= MAX_TASKS) {
    return -1; // Full
  }
  queue->tasks[queue->rear] = task_id;
  queue->rear = (queue->rear + 1) % MAX_TASKS;
  queue->count++;
  return 0;
}

static task_id_t pop_task_from_queue(TaskQueue* queue) {
  if (queue->count == 0) {
    return NO_TASK; // Empty
  }
  task_id_t task_id = queue->tasks[queue->front];
  queue->front = (queue->front + 1) % MAX_TASKS;
  queue->count--;
  return task_id;
}


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value) {
  if (sem == NULL || initial_value > max_value || max_value == 0) {
    return -1;
  }

  memset(sem, 0, sizeof(KSemaphore));
  sem->value = initial_value;
  sem->max_value = max_value;

  return 0;
}

int k_sem_wait(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  
  if (sem->value == 0) {
    push_task_to_queue(&sem->wait_queue, sched_get_task_id());
    spinlock_release(&sem->lock);
    sched_block_task();
    spinlock_acquire(&sem->lock);
  }
  
  sem->value--;
  
  // Wake a waiting poster if any
  task_id_t popped_poster = pop_task_from_queue(&sem->post_queue);
  spinlock_release(&sem->lock);
  
  if (popped_poster != NO_TASK) {
    sched_unblock_task(popped_poster);
  }

  return 0;
}

int k_sem_try_wait(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  if (sem->value == 0) {
    spinlock_release(&sem->lock);
    return -1; // Would block
  }
  sem->value--;
  spinlock_release(&sem->lock);

  return 0;
}


int k_sem_post(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  
  if (sem->value >= sem->max_value) {
    push_task_to_queue(&sem->post_queue, sched_get_task_id());
    spinlock_release(&sem->lock);
    sched_block_task();
    spinlock_acquire(&sem->lock);
  }
  
  sem->value++;
  
  // Wake a waiting waiter if any
  task_id_t popped_waiter = pop_task_from_queue(&sem->wait_queue);
  spinlock_release(&sem->lock);
  
  if (popped_waiter != NO_TASK) {
    sched_unblock_task(popped_waiter);
  }

  return 0;
}

int k_sem_try_post(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  if (sem->value >= sem->max_value) {
    spinlock_release(&sem->lock);
    return -1; // Would block
  }
  sem->value++;
  spinlock_release(&sem->lock);

  return 0;
}

uint64_t k_sem_get_value(KSemaphore* sem) {
  if (sem == NULL) {
    return 0;
  }

  spinlock_acquire(&sem->lock);
  uint64_t value = sem->value;
  spinlock_release(&sem->lock);

  return value;
}