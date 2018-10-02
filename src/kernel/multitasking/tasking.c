/*
 * File: tasking.c
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

#include <main.h>
#include <kprint.h>
#include <string.h>

#include <kernel/tasking.h>
#include <kernel/gdt.h>
#include <kernel/memory/kheap.h>
#include <kernel/main.h>
#include <kernel/timer.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/irqs.h>
#include <kernel/interrupts/smp.h>

#include <kernel/memory/paging.h>
#include <kernel/memory/pmm.h>
#include <kernel/multitasking/syscalls.h>

#include <kernel/lock.h>

extern void _isr_exit(void);
extern void _tasking_thread_exec(void);

static uint32_t nextProcessId = 0;
static uint32_t nextThreadId = 0;

// Thread lists.
static tasking_proc_t *threadLists;

// Locks.
lock_t threadLock = { };
lock_t processLock = { };


bool taskingEnabled = false;

static process_t *kernelProcess = NULL;

static inline void tasking_freeze() {
    taskingEnabled = false;
}

static inline void tasking_unfreeze() {
    taskingEnabled = true;
}

lock_t threadIdLock = { };
static uint32_t tasking_new_thread_id(void) {
    spinlock_lock(&threadIdLock);
    uint32_t threadId = nextThreadId;
    nextThreadId++;
    spinlock_release(&threadIdLock);
    return threadId;
}

lock_t processIdLock = { };
static uint32_t tasking_new_process_id(void) {
    spinlock_lock(&processIdLock);
    uint32_t processId = nextProcessId;
    nextProcessId++;
    spinlock_release(&processIdLock);
    return processId;
}

void tasking_cleanup(void) {
    // Invoke thread cleanup syscall.
    syscalls_syscall(0, 0, 0, 0, 0, 0, SYSCALL_THREAD_CLEANUP);
}

void tasking_cleanup_syscall(void) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t procIndex = (proc != NULL) ? proc->Index : 0;

    // Pause tasking on processor.
    threadLists[procIndex].TaskingEnabled = false;

    // Get current thread.
    thread_t *currentThread = threadLists[procIndex].CurrentThread;

    // Remove thread from schedule.
    currentThread->SchedPrev->SchedNext = currentThread->SchedNext;
    currentThread->SchedNext->SchedPrev = currentThread->SchedPrev;

    // Remove thread from process.
    spinlock_lock(&threadLock);
    currentThread->Prev->Next = currentThread->Next;
    currentThread->Next->Prev = currentThread->Prev;
    spinlock_release(&threadLock);

    // If this is the main thread, we can kill the process too.
    process_t *parentProcess = currentThread->Parent;
    if (currentThread == parentProcess->MainThread) {
        // Remove process from list.
        spinlock_lock(&processLock);
        parentProcess->Prev->Next = parentProcess->Next;
        parentProcess->Next->Prev = parentProcess->Prev;
        spinlock_release(&processLock);

        // Free all threads.
        thread_t *thread = currentThread->Next;
        while (thread != currentThread) {
            kheap_free(thread);
            thread = thread->Next;
        }
        kheap_free(currentThread);

        // Free process from memory.
        kheap_free(parentProcess);
    }
    else {
        // Free thread from memory. The scheduler will move away from it at the next cycle.
        kheap_free(currentThread);
    }

    // Resume tasking.
    threadLists[procIndex].TaskingEnabled = true;
}

void __notified(int sig) {
    // Notify and kill process
    switch(sig) {
        case SIG_ILL:
            kprintf("Received SIGILL, terminating!\n");
            //_kill();
            break;
        case SIG_TERM:
            kprintf("Received SIGTERM, terminating!\n");
           // _kill();
            break;
        case SIG_SEGV:
            kprintf("Received SIGSEGV, terminating!\n");
           // _kill();
            break;
        default:
            kprintf("Received unknown SIG!\n");
            return;
    }
}





thread_t *tasking_thread_create(process_t *process, char *name, thread_entry_func_t func, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2) {
    // Allocate memory for thread.
    thread_t *thread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));

    // Set thread fields.
    thread->Parent = process;
    thread->Name = name;
    thread->ThreadId = tasking_new_thread_id();
    thread->EntryFunc = func;

    // Pop new page for stack and map to temp address.
    thread->StackPage = pmm_pop_frame();
    uintptr_t stackBottom = (uintptr_t)paging_device_alloc(thread->StackPage, thread->StackPage);
    uintptr_t stackTop = stackBottom + PAGE_SIZE_4K;
    memset((void*)stackBottom, 0, PAGE_SIZE_4K);

    // Set up registers.
    irq_regs_t *regs = (irq_regs_t*)(stackTop - sizeof(irq_regs_t));
    regs->FLAGS.AlwaysTrue = true;
    regs->FLAGS.InterruptsEnabled = true;
    regs->CS = process->UserMode ? (GDT_USER_CODE_OFFSET | GDT_PRIVILEGE_USER) : GDT_KERNEL_CODE_OFFSET;
    regs->DS = regs->ES = regs->FS = regs->GS = regs->SS = process->UserMode ? (GDT_USER_DATA_OFFSET | GDT_PRIVILEGE_USER) : GDT_KERNEL_DATA_OFFSET;

    // AX contains the address of thread's main function. BX, CX, and DX contain args.
    regs->IP = (uintptr_t)_tasking_thread_exec;
    regs->AX = (uintptr_t)func;
    regs->BX = arg0;
    regs->CX = arg1;
    regs->DX = arg2;

    // Map stack to lower half if its a user process.
    if (process->UserMode) {
        tasking_freeze();

        // Change to new paging structure.
        uintptr_t oldPagingTablePhys = paging_get_current_directory();
        paging_change_directory(process->PagingTablePhys);

        // Map stack of main thread in.
        paging_map(0x0, thread->StackPage, false, true);
        thread->StackPointer = PAGE_SIZE_4K - sizeof(irq_regs_t);
        regs->SP = regs->BP = PAGE_SIZE_4K;

        // Change back.
        paging_change_directory(oldPagingTablePhys);
        paging_device_free(stackBottom, stackBottom);

        tasking_unfreeze();
    }
    else {
        thread->StackPointer = stackTop - sizeof(irq_regs_t);
        regs->SP = regs->BP = stackTop;
    }

    spinlock_lock(&threadLock);
    // Add thread to process.
    if (process->MainThread != NULL) {
        thread->Next = process->MainThread;
        process->MainThread->Prev->Next = thread;
        thread->Prev = process->MainThread->Prev;
        process->MainThread->Prev = thread;
    }
    else {
        // No existing threads, so add this one as main.
        process->MainThread = thread;
        thread->Next = thread;
        thread->Prev = thread;
    }

    spinlock_release(&threadLock);

    // Return thread.
    return thread;
}

thread_t *tasking_thread_create_kernel(char *name, thread_entry_func_t func, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2) {
    // Create kernel thread.
    return tasking_thread_create(kernelProcess, name, func, arg0, arg1, arg2);
}

process_t *tasking_process_create(process_t *parent, char *name, bool userMode, char *mainThreadName, thread_entry_func_t mainThreadFunc,
    uintptr_t mainThreadArg0, uintptr_t mainThreadArg1, uintptr_t mainThreadArg2) {
    // Allocate memory for process.
    process_t *process = (process_t*)kheap_alloc(sizeof(process_t));
    memset(process, 0, sizeof(process_t));

    // Set up process fields.
    process->Name = name;
    process->Parent = parent;
    process->UserMode = userMode;
    process->PagingTablePhys = userMode ? paging_create_app_copy() : paging_get_current_directory();
    process->ProcessId = nextProcessId;
    nextProcessId++;

    // Create main thread.
    tasking_thread_create(process, mainThreadName, mainThreadFunc, mainThreadArg0, mainThreadArg1, mainThreadArg2);

    // Add to list of system processes.
    spinlock_lock(&processLock);
    if (kernelProcess != NULL) {
        process->Next = kernelProcess;
        kernelProcess->Prev->Next = process;
        process->Prev = kernelProcess->Prev;
        kernelProcess->Prev = process;
    }
    else {
        // No kernel process, so add this one as the kernel process.
        kernelProcess = process;
        process->Next = process;
        process->Prev = process;
    }
    spinlock_release(&processLock);

    // Return process.
    return process;
}

uint32_t tasking_process_get_file_handle(void) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t procIndex = (proc != NULL) ? proc->Index : 0;

    // Pause tasking on processor.
    threadLists[procIndex].TaskingEnabled = false;

    // Get current process.
    uint32_t handle = 0;
    process_t *currentProcess = threadLists[procIndex].CurrentThread->Parent;

    // Get next file handle.
    spinlock_lock(&processLock);
    currentProcess->LastFileHandle++;
    currentProcess->OpenFiles = (vfs_node_t**)kheap_realloc(currentProcess->OpenFiles, sizeof(vfs_node_t*) * (currentProcess->LastFileHandle + 1));
    handle = currentProcess->LastFileHandle;
    spinlock_release(&processLock);

    // Resume tasking on processor and return handle.
    threadLists[procIndex].TaskingEnabled = true;
    return handle;
}

process_t *tasking_process_get_current(void) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t procIndex = (proc != NULL) ? proc->Index : 0;

    // Get current process.
    return threadLists[procIndex].CurrentThread->Parent;
}

void tasking_thread_schedule_proc(thread_t *thread, uint32_t procIndex) {
    // Pause tasking on processor.
    threadLists[procIndex].TaskingEnabled = false;

    // Add thread into schedule on specified processor.
    thread->SchedNext = threadLists[procIndex].CurrentThread->SchedNext;
    thread->SchedNext->SchedPrev = thread;
    thread->SchedPrev = threadLists[procIndex].CurrentThread;
    threadLists[procIndex].CurrentThread->SchedNext = thread;

    // Resume tasking on processor.
    threadLists[procIndex].TaskingEnabled = true;
}

static void kernel_init_thread(void) {
   // while(true) {
        syscalls_kprintf("TASKING: Test from ring 3: %u ticks\n", timer_ticks());
        sleep(1000);
        syscalls_kprintf("TASKING: Done with ring 3 thread.\n");
  //  }
}

static void kernel_main_thread(void) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t procIndex = (proc != NULL) ? proc->Index : 0;

    // Enable tasking on current processor.
    threadLists[procIndex].TaskingEnabled = true;
    kprintf("TASKING: Waiting for all %u processors to come up...\n", smp_get_proc_count());

    // Wait for all processors.
    for (uint32_t i = 0; i < smp_get_proc_count(); i++) {
        // Wait for processor.
        while (!threadLists[i].TaskingEnabled);
    }

    // Start up global tasking and enter into the late kernel.
    kprintf("TASKING: All processors started, enabling multitasking!\n");
    tasking_unfreeze();

    // Create userspace process.
    kprintf("Creating userspace process...\n");
    process_t *initProcess = tasking_process_create(kernelProcess, "init", true, "init_main", kernel_init_thread, 0, 0, 0);
    tasking_thread_schedule_proc(initProcess->MainThread, 0);
    
    kernel_late();
}

static void kernel_idle_thread(uintptr_t procIndex) {
    threadLists[procIndex].TaskingEnabled = true;

    // Do nothing.
    while (true) {
        sleep(1000);
       // kprintf("hi %u\n", lapic_id());
    }
}

static void tasking_exec(uint32_t procIndex) {
    // Send EOI.
    irqs_eoi(0);

    // Change out paging structure and stack.
    paging_change_directory(threadLists[procIndex].CurrentThread->Parent->PagingTablePhys);
#ifdef X86_64
    asm volatile ("mov %0, %%rsp" : : "r"(threadLists[procIndex].CurrentThread->StackPointer));
#else
    asm volatile ("mov %0, %%esp" : : "r"(threadLists[procIndex].CurrentThread->StackPointer));
#endif
    asm volatile ("jmp _irq_exit");
}

void tasking_tick(irq_regs_t *regs, uint32_t procIndex) {
    // Is tasking enabled both globally and for the current processor?
    if (!taskingEnabled || !threadLists[procIndex].TaskingEnabled)
        return;

    // Save stack pointer and move to next thread in schedule.
    threadLists[procIndex].CurrentThread->StackPointer = (uintptr_t)regs;
    threadLists[procIndex].CurrentThread = threadLists[procIndex].CurrentThread->SchedNext;

    // Jump to next task.
    tasking_exec(procIndex);
}

void tasking_init_ap(void) {
    // Disable interrupts, we don't want to screw the following code up.
    interrupts_disable();

    // Get processor.
    smp_proc_t *proc = smp_get_proc(lapic_id());

    // Set kernel stack pointer. This is used for interrupts when switching from ring 3 tasks.
    uintptr_t kernelStack;
#ifdef X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(gdt_tss_get(), kernelStack);

    // Initialize fast syscalls for this processor.
    syscalls_init_ap();

    // Create idle kernel thread.
    thread_t *idleThread = tasking_thread_create_kernel("core_idle", kernel_idle_thread, proc->Index, 0, 0);
    threadLists[proc->Index].CurrentThread = idleThread;
    idleThread->SchedNext = idleThread;
    idleThread->SchedPrev = idleThread;

    // Start tasking!
    interrupts_enable();
    tasking_exec(proc->Index);
}

void tasking_init(void) {
    // Disable interrupts, we don't want to screw the following code up.
    interrupts_disable();

    // Set kernel stack pointer. This is used for interrupts when switching from ring 3 tasks.
    uintptr_t kernelStack;
#ifdef X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(NULL, kernelStack);

    // Initialize system calls.
    syscalls_init();

    // Create thread lists for processors.
    threadLists = (tasking_proc_t*)kheap_alloc(sizeof(tasking_proc_t) * smp_get_proc_count());
    memset(threadLists, 0, sizeof(tasking_proc_t) * smp_get_proc_count());

    // Create main kernel process.
    kprintf("Creating kernel process...\n");
    tasking_process_create(NULL, "kernel", false, "kernel_main", kernel_main_thread, 0, 0, 0);
    threadLists[0].CurrentThread = kernelProcess->MainThread;
    kernelProcess->MainThread->SchedNext = kernelProcess->MainThread;
    kernelProcess->MainThread->SchedPrev = kernelProcess->MainThread;

    // Start tasking on BSP!
    interrupts_enable();
    tasking_exec(0);
}
