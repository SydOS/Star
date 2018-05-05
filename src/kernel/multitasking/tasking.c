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

#include <kernel/lock.h>

extern void _isr_exit(void);
extern void _tasking_thread_exec(void);

static uint32_t nextProcessId = 0;
static uint32_t nextThreadId = 0;

// Thread lists.
static tasking_proc_t *threadLists;


// Current process.
//Process* currentProcess = 0;
uint32_t lpid = 0;
bool taskingEnabled = false;

static process_t *currentProcess = NULL;
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

void tasking_kill_thread(void) {
    // Pause tasking.
    tasking_freeze();

    // If this is the last or main thread, we can just kill the process.
    if (currentProcess->CurrentThread == currentProcess->CurrentThread->Next || currentProcess->CurrentThread == currentProcess->MainThread) {
        // Remove process from list.
        currentProcess->Prev->Next = currentProcess->Next;
        currentProcess->Next->Prev = currentProcess->Prev;

        // Free all threads.
        thread_t *thread = currentProcess->CurrentThread->Next;
        while (thread != currentProcess->CurrentThread) {
            kheap_free(thread);
            thread = thread->Next;
        }
        kheap_free(currentProcess->CurrentThread);

        // Free process from memory.
        kheap_free(currentProcess);
    }
    else {
        // Remove thread from list.
        currentProcess->CurrentThread->Prev->Next = currentProcess->CurrentThread->Next;
        currentProcess->CurrentThread->Next->Prev = currentProcess->CurrentThread->Prev;

        // Free thread from memory. The scheduler will move away from it at the next cycle.
        kheap_free(currentProcess->CurrentThread);
    }

    // Resume tasking.
    tasking_unfreeze();
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

void tasking_fork(void) {

}

thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    // Allocate memory for thread.
    thread_t *thread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->Name = name;

    // Pop new page for stack and map to temp address.
    thread->StackPage = pmm_pop_frame();
    thread->StackPointer = (uintptr_t)paging_device_alloc(thread->StackPage, thread->StackPage) + PAGE_SIZE_4K;

    // Set up registers.
    uintptr_t stackTop = thread->StackPointer;
    thread->StackPointer -= sizeof(irq_regs_t);
    /*thread->Regs = (irq_regs_t*)thread->StackPointer;
    thread->Regs->SP = thread->Regs->BP = stackTop;
    thread->Regs->FLAGS.AlwaysTrue = true;
    thread->Regs->FLAGS.InterruptsEnabled = true;

    // AX contains the address of thread's main function.
    thread->Regs->IP = _tasking_thread_exec;
    thread->Regs->AX = addr;

    // BX, CX, and DX contain args to be passed to that function, in order.
    thread->Regs->BX = arg1;
    thread->Regs->CX = arg2;
    thread->Regs->DX = arg3;*/
    
    // Return the thread.
    thread->Next = thread;
    thread->Prev = thread;
    return thread;
}

uint32_t tasking_thread_add(thread_t *thread, process_t *process) {
    // Pause tasking.
    tasking_freeze();

    // Set segments for thread.
  /*  thread->Regs->CS = process->KernelMode ? GDT_KERNEL_CODE_OFFSET : (GDT_USER_CODE_OFFSET | GDT_SELECTOR_RPL_RING3);
    thread->Regs->DS = thread->Regs->ES = thread->Regs->FS = thread->Regs->GS = thread->Regs->SS =
        process->KernelMode ? GDT_KERNEL_DATA_OFFSET : (GDT_USER_DATA_OFFSET | GDT_SELECTOR_RPL_RING3);*/

    // Add thread into process schedule.
    thread->Next = process->CurrentThread->Next;
    thread->Next->Prev = thread;
    thread->Prev = process->CurrentThread;
    process->CurrentThread->Next = thread;

    // Resume tasking.
    tasking_unfreeze();

    // Return the thread ID.
    return thread->ThreadId;
}

uint32_t tasking_thread_add_kernel(thread_t *thread) {
    return tasking_thread_add(thread, kernelProcess);
}

process_t* tasking_process_create(char* name, thread_t *mainThread, bool kernel) {
    // Allocate memory for process.
    process_t* process = (process_t*)kheap_alloc(sizeof(process_t));
    memset(process, 0, sizeof(process_t));

    // Set up process fields.
    process->Name = name;
  //  process->KernelMode = kernel;
    //process->Pid = ++lpid;
    //process->State = PROCESS_STATE_ALIVE;
    //process->Notify = __notified;

    // Set segments for thread.
   /* mainThread->Regs->CS = process->KernelMode ? GDT_KERNEL_CODE_OFFSET : (GDT_USER_CODE_OFFSET | GDT_SELECTOR_RPL_RING3);
    mainThread->Regs->DS = mainThread->Regs->ES = mainThread->Regs->FS = mainThread->Regs->GS = mainThread->Regs->SS =
        process->KernelMode ? GDT_KERNEL_DATA_OFFSET : (GDT_USER_DATA_OFFSET | GDT_SELECTOR_RPL_RING3);*/

    process->MainThread = mainThread;
    process->CurrentThread = process->MainThread;
    
    interrupts_disable();
    // Create a new root paging structure, with the higher half of the kernel copied in.
    //process->PagingTablePhys = paging_create_app_copy();

    // Change to new paging structure.
    uintptr_t oldPagingTablePhys = paging_get_current_directory();
    paging_change_directory(process->PagingTablePhys);

    // Map stack of main thread in.
    paging_map(0x0, process->MainThread->StackPage, false, true);
    process->MainThread->StackPointer = PAGE_SIZE_4K - sizeof(irq_regs_t);
  //  process->MainThread->Regs->SP = process->MainThread->Regs->BP = PAGE_SIZE_4K;

    // Change back.
    paging_change_directory(oldPagingTablePhys);
    interrupts_enable();
    return process;
}

uint32_t tasking_process_add(process_t *process) {
    // Pause tasking.
    tasking_freeze();

    // Add process into schedule.
    process->Next = currentProcess->Next;
    process->Next->Prev = process;
    process->Prev = currentProcess;
    currentProcess->Next = process;

    // Resume tasking.
    tasking_unfreeze();

    // Return the PID.
    return process->ProcessId;
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

static void kernel_main_thread(void) {
    // Start up tasking and enter into the late kernel.
    tasking_unfreeze();
    kernel_late();
}

static void kernel_idle_thread(uintptr_t procIndex) {
    threadLists[procIndex].TaskingEnabled = true;

    // Do nothing.
    while (true) {
        sleep(1000);
        kprintf("hi %u\n", lapic_id());
    }
}

static void kernel_init_thread(void) {
    while(true) {
        syscalls_kprintf("Test LAPIC #%u: %u ticks\n", lapic_id(), timer_ticks());
        sleep(1000);
    }
}
lock_t threadLock = { };

static thread_t *tasking_t_create(process_t *process, char *name, void *func, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2) {
    // Allocate memory for thread.
    thread_t *thread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));

    // Set thread fields.
    thread->Parent = process;
    thread->Name = name;
    thread->ThreadId = tasking_new_thread_id();

    // Pop new page for stack and map to temp address.
    thread->StackPage = pmm_pop_frame();
    uintptr_t stackBottom = (uintptr_t)paging_device_alloc(thread->StackPage, thread->StackPage);
    uintptr_t stackTop = stackBottom + PAGE_SIZE_4K;
    memset((void*)stackBottom, 0, PAGE_SIZE_4K);

    // Set up registers.
    irq_regs_t *regs = (irq_regs_t*)(stackTop - sizeof(irq_regs_t));
    regs->FLAGS.AlwaysTrue = true;
    regs->FLAGS.InterruptsEnabled = true;
    regs->CS = process->UserMode ? GDT_USER_CODE_OFFSET : GDT_KERNEL_CODE_OFFSET;
    regs->DS = regs->ES = regs->FS = regs->GS = regs->SS = process->UserMode ? GDT_USER_DATA_OFFSET : GDT_KERNEL_DATA_OFFSET;

    // AX contains the address of thread's main function. BX, CX, and DX contain args.
    regs->IP = (uintptr_t)_tasking_thread_exec;
    regs->AX = (uintptr_t)func;
    regs->BX = arg0;
    regs->CX = arg1;
    regs->DX = arg2;

    // Map stack to lower half if its a user process.
    if (process->UserMode) {

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
        process->Prev = process->MainThread->Prev;
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

process_t *tasking_p_create(process_t *parent, char *name, bool userMode, char *mainThreadName, void *mainThreadFunc,
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
    tasking_t_create(process, mainThreadName, mainThreadFunc, mainThreadArg0, mainThreadArg1, mainThreadArg2);

    // Add to list of system processes.
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

    // Return process.
    return process;
}

void tasking_t_schedule(thread_t *thread, uint32_t procIndex) {
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

void tasking_init_ap(void) {
    // Disable interrupts, we don't want to screw the following code up.
    interrupts_disable();

    // Get processor.
    smp_proc_t *proc = smp_get_proc(lapic_id());

    // Initialize fast syscalls for this processor.
    syscalls_init_ap();

    // Set kernel stack pointer. This is used for interrupts when switching from ring 3 tasks.
    uintptr_t kernelStack;
#ifdef X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(gdt_tss_get(), kernelStack);

    // Create idle kernel thread.
    thread_t *idleThread = tasking_t_create(kernelProcess, "core_idle", kernel_idle_thread, proc->Index, 0, 0);
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

    // Initialize system calls.
    syscalls_init();

    // Set kernel stack pointer. This is used for interrupts when switching from ring 3 tasks.
    uintptr_t kernelStack;
#ifdef X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(NULL, kernelStack);

    // Create thread lists for processors.
    threadLists = (tasking_proc_t*)kheap_alloc(sizeof(tasking_proc_t) * smp_get_proc_count());
    memset(threadLists, 0, sizeof(tasking_proc_t) * smp_get_proc_count());

    // Create main kernel process.
    kprintf("Creating kernel process...\n");
    tasking_p_create(NULL, "kernel", false, "kernel_main", kernel_main_thread, 0, 0, 0);
    threadLists[0].CurrentThread = kernelProcess->MainThread;
    kernelProcess->MainThread->SchedNext = kernelProcess->MainThread;
    kernelProcess->MainThread->SchedPrev = kernelProcess->MainThread;

    // Create userspace process.
    kprintf("Creating userspace process...\n");
    process_t *initProcess = tasking_p_create(kernelProcess, "init", true, "init_main", kernel_init_thread, 0, 0, 0);

    // Start tasking on BSP!
    interrupts_enable();
    threadLists[0].TaskingEnabled = true;
    tasking_exec(0);
}
