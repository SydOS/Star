#ifndef LOCK_H
#define LOCK_H

#include <main.h>

typedef volatile uintptr_t lock_t;

extern void spinlock_lock(lock_t *lockObject);
extern void spinlock_release(lock_t *lockObject);

#endif
