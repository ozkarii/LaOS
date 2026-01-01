#include <stddef.h>
#include "sem.h"


static int push_task_to_wait_queue(KSemaphore* sem, task_id_t task_id) {
  if (sem->wait_count >= MAX_TASKS) {
    return -1; // Full
  }
  sem->wait_queue[sem->wait_count++] = task_id;
  return 0;
}

static task_id_t pop_task_from_wait_queue(KSemaphore* sem) {
  if (sem->wait_count == 0) {
    return NO_TASK; // Empty
  }
  task_id_t task_id = sem->wait_queue[0];
  // Shift remaining tasks
  // TODO: ring buffer
  for (uint32_t i = 1; i < sem->wait_count; i++) {
    sem->wait_queue[i - 1] = sem->wait_queue[i];
  }
  sem->wait_count--;

  return task_id;
}


int k_sem_init(KSemaphore* sem, uint64_t initial_value, uint64_t max_value) {
  sem->value = initial_value;
  sem->max_value = max_value;
  return 0;
}

int k_sem_wait(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  if (sem->value == 0) {
    push_task_to_wait_queue(sem, sched_get_task_id());
    sched_block_task();
  }
  sem->value--;

  return 0;
}

int k_sem_post(KSemaphore* sem) {
  if (sem == NULL) {
    return -1;
  }

  if (sem->value < sem->max_value) {
    sem->value++;
  }

  task_id_t popped_task = pop_task_from_wait_queue(sem);
  if (popped_task != NO_TASK) {
    sched_unblock_task(popped_task);
  }

  return 0;
}
