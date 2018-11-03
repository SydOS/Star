/*
 * File: syscalls.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <main.h>

#define SYSCALL_INTERRUPT   0x80

#define SYSCALL_MSR_EFER    0xC0000080
#define SYSCALL_MSR_STAR    0xC0000081
#define SYSCALL_MSR_LSTAR   0xC0000082
#define SYSCALL_MSR_CSTAR   0xC0000083
#define SYSCALL_MSR_SFMASK  0xC0000084

#define SYSCALL_MSR_SYSENTER_CS     0x174
#define SYSCALL_MSR_SYSENTER_ESP    0x175
#define SYSCALL_MSR_SYSENTER_EIP    0x176

// Syscall definitions.
#define SYSCALL_READ    0x00
#define SYSCALL_OPEN 0x02
#define SYSCALL_GET_DIR_ENTRIES 0x4E
#define SYSCALL_UPTIME 0x15

#define SYSCALL_THREAD_CLEANUP 0xFFFE

extern uintptr_t syscalls_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);

extern void syscalls_kprintf(const char *format, ...);

#endif
