#ifndef LOCK_H
#define LOCK_H

#include <main.h>

typedef volatile struct {
    uintptr_t Lock;
    uintptr_t InterruptState;
} __attribute__((packed)) lock_t;

extern void spinlock_lock(lock_t *lockObject);
extern void spinlock_release(lock_t *lockObject);

#endif
