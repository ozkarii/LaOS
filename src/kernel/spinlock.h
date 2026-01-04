#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

typedef struct Spinlock {
  _Atomic uint32_t locked;
} Spinlock;

void spinlock_acquire(Spinlock* lock);
void spinlock_release(Spinlock* lock);

#endif /* SPINLOCK_H */