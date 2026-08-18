#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "armv8-a.h"

#define SPINLOCK_INIT {0, false, {0}}
#define SPINLOCK_INIT_IRQ_SAFE {0, true, {0}}

typedef struct Spinlock {
  _Atomic uint32_t locked;
  bool used_from_irq;
  uint64_t saved_irq_state[NUM_CPUS];
} Spinlock;

// Not reentrant
void spinlock_acquire(Spinlock* lock);
void spinlock_release(Spinlock* lock);

#endif /* SPINLOCK_H */