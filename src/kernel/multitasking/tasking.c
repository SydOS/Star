#include <main.h>
#include <kprint.h>
#include <string.h>

#include <kernel/tasking.h>
#include <kernel/gdt.h>
#include <kernel/memory/kheap.h>
#include <kernel/main.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/irqs.h>

extern void _isr_exit(void);
extern void _tasking_thread_exec(void);

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

thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    // Allocate memory for thread.
    thread_t *thread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->Name = name;

    // Set up registers.
    uintptr_t stackTop = (uintptr_t)thread->Stack + THREAD_STACK_SIZE;
    thread->StackPointer = stackTop - sizeof(irq_regs_t);
    thread->Regs = (irq_regs_t*)thread->StackPointer;
    thread->Regs->SP = thread->Regs->BP = stackTop;
    thread->Regs->FLAGS.AlwaysTrue = true;
    thread->Regs->FLAGS.InterruptsEnabled = true;

    // AX contains the address of thread's main function.
    thread->Regs->IP = _tasking_thread_exec;
    thread->Regs->AX = addr;

    // BX, CX, and DX contain args to be passed to that function, in order.
    thread->Regs->BX = arg1;
    thread->Regs->CX = arg2;
    thread->Regs->DX = arg3;
    
    // Return the thread.
    thread->Next = thread;
    thread->Prev = thread;
    return thread;
}

uint32_t tasking_thread_add(thread_t *thread, process_t *process) {
    // Pause tasking.
    tasking_freeze();

    // Set segments for thread.
    thread->Regs->CS = process->KernelMode ? GDT_KERNEL_CODE_OFFSET : (GDT_USER_CODE_OFFSET | GDT_SELECTOR_RPL_RING3);
    thread->Regs->DS = thread->Regs->ES = thread->Regs->FS = thread->Regs->GS = thread->Regs->SS =
        process->KernelMode ? GDT_KERNEL_DATA_OFFSET : (GDT_USER_DATA_OFFSET | GDT_SELECTOR_RPL_RING3);

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
    process->KernelMode = kernel;
    //process->Pid = ++lpid;
    //process->State = PROCESS_STATE_ALIVE;
    //process->Notify = __notified;

    // Set segments for thread.
    mainThread->Regs->CS = process->KernelMode ? GDT_KERNEL_CODE_OFFSET : (GDT_USER_CODE_OFFSET | GDT_SELECTOR_RPL_RING3);
    mainThread->Regs->DS = mainThread->Regs->ES = mainThread->Regs->FS = mainThread->Regs->GS = mainThread->Regs->SS =
        process->KernelMode ? GDT_KERNEL_DATA_OFFSET : (GDT_USER_DATA_OFFSET | GDT_SELECTOR_RPL_RING3);

    process->MainThread = mainThread;
    process->CurrentThread = process->MainThread;
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
    if (pit_ticks() % 2)
        currentProcess = currentProcess->Next;

    // Jump to next task.
    tasking_exec();
}

static void kernel_main_thread(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    tasking_unfreeze();
    kernel_late();
}

void tasking_init(void) {
    // Initialize syscalls.
    syscalls_init();

    // Disable interrupts, we don't want to screw the following code up.
    interrupts_disable();

    // Create kernel thread and process.
    kprintf("Creating kernel process...\n");
    thread_t *kernelThread = tasking_thread_create("kernel_main", (uintptr_t)kernel_main_thread, 10, 20, 30);
    kernelProcess = tasking_process_create("kernel", kernelThread, true);
    kernelProcess->Next = kernelProcess;
    kernelProcess->Prev = kernelProcess;
    currentProcess = kernelProcess;

    // Set kernel stack pointer.
    uintptr_t kernelStack;
#ifdef X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(kernelStack);

    // Start tasking!
    interrupts_enable();
    tasking_exec();
}
