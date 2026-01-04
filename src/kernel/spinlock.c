#include "armv8-a.h"
#include "spinlock.h"

void spinlock_acquire(Spinlock* lock) {
#if USE_SMP
  while (atomic_exchange(&lock->locked, 1) == 1) {
    __asm__ __volatile__("yield");
  }
#else
  (void)lock;
  DISABLE_ALL_INTERRUPTS();
#endif
}

void spinlock_release(Spinlock* lock) {
#if USE_SMP
  atomic_store(&lock->locked, 0);
#else
  (void)lock;
  ENABLE_ALL_INTERRUPTS();
#endif
}