#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <main.h>

#define SYSCALL_UPTIME 0x15

extern uintptr_t syscalls_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);

extern void syscalls_kprintf(const char *format, ...);

#endif
