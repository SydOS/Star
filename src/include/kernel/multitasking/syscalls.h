#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <main.h>

#define SYSCALL_MSR_EFER    0xC0000080
#define SYSCALL_MSR_STAR    0xC0000081
#define SYSCALL_MSR_LSTAR   0xC0000082
#define SYSCALL_MSR_CSTAR   0xC0000083
#define SYSCALL_MSR_SFMASK  0xC0000084

#define SYSCALL_MSR_SYSENTER_CS     0x174
#define SYSCALL_MSR_SYSENTER_ESP    0x175
#define SYSCALL_MSR_SYSENTER_EIP    0x176

#define SYSCALL_UPTIME 0x15

extern uintptr_t syscalls_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);

extern void syscalls_kprintf(const char *format, ...);

#endif
