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

extern void _isr_exit(void);
extern void _tasking_thread_exec(void);

// Thread lists.
static thread_t **threadLists;


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

static void tasking_exec() {
    // Send EOI and change out stacks.
    irqs_eoi(0);
#ifdef X86_64
    asm volatile ("mov %0, %%rsp" : : "r"(currentProcess->CurrentThread->StackPointer));
#else
    asm volatile ("mov %0, %%esp" : : "r"(currentProcess->CurrentThread->StackPointer));
#endif
    asm volatile ("jmp _irq_exit");
}

void tasking_tick(irq_regs_t *regs) {
    // Is tasking enabled?
    if (!taskingEnabled)
        return;

    // Save stack pointer and change to the next thread.
    currentProcess->CurrentThread->StackPointer = (uintptr_t)regs;
    currentProcess->CurrentThread = currentProcess->CurrentThread->Next;

    // Every other tick, change to next process.
    if (timer_ticks() % 2) {
        currentProcess = currentProcess->Next;
        if (currentProcess->PagingTablePhys != 0)
            paging_change_directory(currentProcess->PagingTablePhys);
    }

    // Jump to next task.
    tasking_exec();
}

static void kernel_main_thread(void) {
    // Start up tasking and enter into the late kernel.
    tasking_unfreeze();
    kernel_late();
}

static void kernel_init_thread(void) {
    while(true) {
        syscalls_kprintf("Test\n");
        sleep(2000);
    }
}

static void tasking_create_kernel_process(void) {
    // Allocate memory for kernel process.
    kernelProcess = (process_t*)kheap_alloc(sizeof(process_t));
    memset(kernelProcess, 0, sizeof(process_t));

    // Set up process fields.
    kernelProcess->Name = "kernel";
    kernelProcess->ProcessId = 0;
    kernelProcess->PagingTablePhys = paging_get_current_directory();
    kernelProcess->Next = kernelProcess;
    kernelProcess->Prev = kernelProcess;
    currentProcess = kernelProcess;

    // Allocate memory for thread.
    thread_t *kernelThread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(kernelThread, 0, sizeof(thread_t));

    // Set thread fields.
    kernelThread->Name = "kernel_main";
    kernelThread->ThreadId = 0;
    kernelThread->Next = kernelThread;
    kernelThread->Prev = kernelThread;

    // Pop new page for stack and map to temp address.
    kernelThread->StackPage = pmm_pop_frame();
    uintptr_t stackBottom = (uintptr_t)paging_device_alloc(kernelThread->StackPage, kernelThread->StackPage);
    uintptr_t stackTop = stackBottom + PAGE_SIZE_4K;
    memset((void*)stackBottom, 0, PAGE_SIZE_4K);

    // Set up registers.
    irq_regs_t *regs = (irq_regs_t*)(stackTop - sizeof(irq_regs_t));
    regs->FLAGS.AlwaysTrue = true;
    regs->FLAGS.InterruptsEnabled = true;
    regs->CS = GDT_KERNEL_CODE_OFFSET;
    regs->DS = regs->ES = regs->FS = regs->GS = regs->SS = GDT_KERNEL_DATA_OFFSET;

    // AX contains the address of thread's main function.
    regs->IP = _tasking_thread_exec;
    regs->AX = kernel_main_thread;

    // Map stack to 0x0 for now.
    //paging_map(0x0, kernelThread->StackPage, false, true);
    kernelThread->StackPointer = stackTop - sizeof(irq_regs_t);
    regs->SP = regs->BP = stackTop;
    //paging_device_free(stackBottom, stackBottom);

    // Thread is the only one in process.
    kernelProcess->MainThread = kernelThread;
    kernelProcess->CurrentThread = kernelProcess->MainThread;
}

static void tasking_create_init_process(void) {
    // Allocate memory for kernel process.
    process_t *initProcess = (process_t*)kheap_alloc(sizeof(process_t));
    memset(initProcess, 0, sizeof(process_t));

    // Set up process fields.
    initProcess->Name = "init";
    initProcess->ProcessId = 1;
    initProcess->Next = currentProcess->Next;
    initProcess->Next->Prev = initProcess;
    initProcess->Prev = currentProcess;
    currentProcess->Next = initProcess;

    // Allocate memory for thread.
    thread_t *initThread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(initThread, 0, sizeof(thread_t));

    // Set thread fields.
    initThread->Name = "init_main";
    initThread->ThreadId = 1;
    initThread->Next = initThread;
    initThread->Prev = initThread;

    // Pop new page for stack and map to temp address.
    initThread->StackPage = pmm_pop_frame();
    uintptr_t stackBottom = (uintptr_t)paging_device_alloc(initThread->StackPage, initThread->StackPage);
    uintptr_t stackTop = stackBottom + PAGE_SIZE_4K;
    memset((void*)stackBottom, 0, PAGE_SIZE_4K);

    // Set up registers.
    irq_regs_t *regs = (irq_regs_t*)(stackTop - sizeof(irq_regs_t));
    regs->FLAGS.AlwaysTrue = true;
    regs->FLAGS.InterruptsEnabled = true;
    regs->CS = GDT_USER_CODE_OFFSET | GDT_SELECTOR_RPL_RING3;
    regs->DS = regs->ES = regs->FS = regs->GS = regs->SS = (GDT_USER_DATA_OFFSET | GDT_SELECTOR_RPL_RING3);

    // AX contains the address of thread's main function.
    regs->IP = _tasking_thread_exec;
    regs->AX = kernel_init_thread;

    // Create a new root paging structure, with the higher half of the kernel copied in.
    initProcess->PagingTablePhys = paging_create_app_copy();

    // Change to new paging structure.
    uintptr_t oldPagingTablePhys = paging_get_current_directory();
    paging_change_directory(initProcess->PagingTablePhys);

    // Map stack of main thread in.
    paging_map(0x0, initThread->StackPage, false, true);
    initThread->StackPointer = PAGE_SIZE_4K - sizeof(irq_regs_t);
    regs->SP = regs->BP = PAGE_SIZE_4K;

    // Change back.
    paging_change_directory(oldPagingTablePhys);
    paging_device_free(stackBottom, stackBottom);

    // Thread is the only one in process.
    initProcess->MainThread = initThread;
    initProcess->CurrentThread = initProcess->MainThread;
}

void tasking_init(void) {
        while(true);
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
    threadLists = (thread_t**)kheap_alloc(sizeof(thread_t) * smp_get_proc_count());
    memset(threadLists, 0, sizeof(thread_t) * smp_get_proc_count());

    // Create kernel thread 0 and process 0.
    kprintf("Creating kernel process...\n");
    tasking_create_kernel_process();

    // Create main user process 1.
    tasking_create_init_process();

    // Start tasking!

    interrupts_enable();
    tasking_exec();
}
