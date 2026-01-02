#include <stddef.h>
#include "string.h"
#include "sem.h"


static int push_task_to_wait_queue(KSemaphore* sem, task_id_t task_id) {
  if (sem->wait_count >= MAX_TASKS) {
    return -1; // Full
  }
  sem->wait_queue[sem->queue_rear] = task_id;
  sem->queue_rear = (sem->queue_rear + 1) % MAX_TASKS;
  sem->wait_count++;
  return 0;
}

static task_id_t pop_task_from_wait_queue(KSemaphore* sem) {
  if (sem->wait_count == 0) {
    return NO_TASK; // Empty
  }
  task_id_t task_id = sem->wait_queue[sem->queue_front];
  sem->queue_front = (sem->queue_front + 1) % MAX_TASKS;
  sem->wait_count--;

  return task_id;
}


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value) {
  if (sem == NULL || initial_value > max_value || max_value == 0) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  sem->value = initial_value;
  sem->max_value = max_value;
  memset(sem->wait_queue, 0, sizeof(sem->wait_queue));
  sem->wait_count = 0;
  sem->queue_front = 0;
  sem->queue_rear = 0;
  spinlock_release(&sem->lock);

  return 0;
}

int k_sem_wait(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  spinlock_acquire(&sem->lock);
  if (sem->value == 0) {
    push_task_to_wait_queue(sem, sched_get_task_id());
    spinlock_release(&sem->lock);
    sched_block_task();
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
  if (sem->value < sem->max_value) {
    sem->value++;
  }

  task_id_t popped_task = pop_task_from_wait_queue(sem);
  if (popped_task != NO_TASK) {
    sched_unblock_task(popped_task);
  }
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