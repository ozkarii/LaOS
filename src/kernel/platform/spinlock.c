#include "spinlock.h"

void spinlock_acquire(Spinlock* lock) {
#if USE_SMP
  if (lock->used_from_irq) {
    lock->saved_irq_state[GET_CPU_ID()] = GET_DAIF();
    MASK_ALL_INTERRUPTS();
  }
  while (atomic_exchange(&lock->locked, 1) == 1) {
    __asm__ __volatile__("yield");
  }
#else
  (void)lock;
  MASK_ALL_INTERRUPTS();
#endif
}

void spinlock_release(Spinlock* lock) {
#if USE_SMP
  atomic_store(&lock->locked, 0);
  if (lock->used_from_irq) {
    SET_DAIF(lock->saved_irq_state[GET_CPU_ID()]);
  }
#else
  (void)lock;
  UNMASK_ALL_INTERRUPTS();
#endif
}