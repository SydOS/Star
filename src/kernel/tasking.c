#include <main.h>
#include <kprint.h>
#include <string.h>

#include <kernel/tasking.h>
#include <kernel/memory/kheap.h>
#include <kernel/main.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/irqs.h>

extern void _isr_exit();

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

void _kill() {
    // Pause tasking.
    tasking_freeze();

    // Remove task.
    currentProcess->Prev->Next = currentProcess->Next;
    currentProcess->Next->Prev = currentProcess->Prev;

    // Free memory.
    //kheap_free((void *)currentProcess->StackBottom);
   // kheap_free(currentProcess);

    // Resume tasking.
    tasking_unfreeze();
}

void __notified(int sig) {
    // Notify and kill process
    switch(sig) {
        case SIG_ILL:
            kprintf("Received SIGILL, terminating!\n");
            _kill();
            break;
        case SIG_TERM:
            kprintf("Received SIGTERM, terminating!\n");
            _kill();
            break;
        case SIG_SEGV:
            kprintf("Received SIGSEGV, terminating!\n");
            _kill();
            break;
        default:
            kprintf("Received unknown SIG!\n");
            return;
    }
}

// -----------------------------------------------------------------------------

int tasking_add_process(Process* newProcess) {
    // Pause tasking.
   // tasking_freeze();

    // Add process into mix.
 //   newProcess->Next = currentProcess->Next;
 //   newProcess->Next->Prev = newProcess;
 //   newProcess->Prev = currentProcess;
 //   currentProcess->Next = newProcess;

    // Resume tasking.
    //tasking_unfreeze();

    // Return the PID.
    return newProcess->Pid;
}

thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t ecx, uintptr_t edx, bool userLevel) {
    // Allocate memory for process.
    thread_t *thread = (thread_t*)kheap_alloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->Name = name;

    // Allocate space for stack.
    thread->StackBottom = (uintptr_t)kheap_alloc(4096);
    thread->StackTop = thread->StackBottom + 4096;
    thread->User = userLevel;
    uintptr_t stackTop = thread->StackTop;
    memset((void*)thread->StackBottom, 0, 4096);

    thread->SS = userLevel ? 0x23 : 0x10;

    // Set up registers.
    stackTop -= sizeof(irq_regs_t);
    thread->SP = stackTop;
    thread->Regs = (irq_regs_t*)stackTop;
    thread->Regs->FLAGS = 0x202;
    thread->Regs->IP = addr;
    thread->Regs->BP = thread->StackTop;
    thread->Regs->SP = thread->StackTop;
    thread->Regs->SS = thread->SS;
    thread->Regs->CX = ecx;	
    thread->Regs->DX = edx;	
    thread->Regs->CS = userLevel ? 0x1B : 0x8;
    thread->Regs->DS = userLevel ? 0x23 : 0x10;
    thread->Regs->FS = userLevel ? 0x23 : 0x10;
    thread->Regs->ES = userLevel ? 0x23 : 0x10;
    thread->Regs->GS = userLevel ? 0x23 : 0x10;

    // Return the thread.
    thread->Next = thread;
    thread->Prev = thread;
    return thread;
}

uint32_t tasking_thread_add(thread_t *thread, process_t *process) {
    // Pause tasking.
    tasking_freeze();

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

process_t* tasking_process_create(char* name, thread_t *mainThread) {
    // Allocate memory for process.
    process_t* process = (process_t*)kheap_alloc(sizeof(process_t));
    memset(process, 0, sizeof(process_t));

    // Set up process fields.
    process->Name = name;
    //process->Pid = ++lpid;
    //process->State = PROCESS_STATE_ALIVE;
    //process->Notify = __notified;
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

Process* tasking_create_process(char* name, uintptr_t addr, uintptr_t ecx, uintptr_t edx) {
    // Allocate memory for process.
  /*  Process* process = (Process*)kheap_alloc(sizeof(Process));
    memset(process, 0, sizeof(Process));

    // Set up process fields.
    process->Name = name;
    process->Pid = ++lpid;
    process->State = PROCESS_STATE_ALIVE;
    process->Notify = __notified;

    // Allocate space for stack.
    process->StackBottom = (uintptr_t)kheap_alloc(4096);
    process->StackTop = process->StackBottom + 4096;
    uintptr_t stackTop = process->StackTop;
    memset((void*)process->StackBottom, 0, 4096);

    // Set up registers.
    process->StackTop -= sizeof(irq_regs_t);
    process->Regs = (irq_regs_t*)process->StackTop;

    process->Regs->flags = 0x202;
    process->Regs->ip = addr;
    process->Regs->bp = stackTop;
    //process->Regs->sp = process->StackTop;
    process->Regs->cx = ecx;	
    process->Regs->dx = edx;	

    process->Regs->cs = 0x8;
    process->Regs->ds = 0x10;
    process->Regs->fs = 0x10;
    process->Regs->es = 0x10;
    process->Regs->gs = 0x10;*/
   // return process;
}

static void tasking_exec() {
    

    // Send EOI and change out stacks.
    irqs_eoi(0);
#ifdef X86_64
    asm volatile ("mov %0, %%rsp" : : "r"(currentProcess->CurrentThread->SP));
#else
    //asm volatile ("mov %0, %%ebx" : : "r"(currentProcess->CurrentThread->Regs->ip));
    asm volatile ("mov %0, %%esp" : : "r"(currentProcess->CurrentThread->SP));
    //asm volatile ("mov %0, %%ebp" : : "r"(currentProcess->CurrentThread->Regs->bp));
   // asm volatile ("jmp *%%ebx" : : );
#endif
    asm volatile ("jmp _irq_exit");
}

void tasking_tick(irq_regs_t *regs) {
    // Is tasking enabled?
    if (!taskingEnabled)
        return;

    // Save registers and change to the next thread.
    currentProcess->CurrentThread->SP = (uintptr_t)regs;
    //currentProcess->CurrentThread->Regs = regs;
    currentProcess->CurrentThread = currentProcess->CurrentThread->Next;
    irq_regs_t *newRegs = (irq_regs_t *)currentProcess->CurrentThread->SP;

    // Every other tick, change to next process.
   // if (pit_ticks() % 2)
  //      currentProcess = currentProcess->Next;
    //tasking_freeze();

    if (currentProcess->CurrentThread->User) {
        //interrupts_disable();
       // currentProcess->CurrentThread->Regs->flags = 0x2;
    }
    // Jump to next task.
    tasking_exec();
}

static void kernel_main_thread(void) {
    tasking_unfreeze();
    kernel_late();
}

void tasking_init(void) {
    // Disable interrupts, we don't want to screw the following code up.
    interrupts_disable();

    // Create kernel thread.
    kprintf("Creating kernel thread...\n");
    thread_t *kernelThread = tasking_thread_create("kernel_main", (uintptr_t)kernel_main_thread, 0, 0, false);
    kernelProcess = tasking_process_create("kernel", kernelThread);
    kernelProcess->Next = kernelProcess;
    kernelProcess->Prev = kernelProcess;
    currentProcess = kernelProcess;

    // Set kernel stack pointer.
    uintptr_t kernelStack;
#if X86_64
    asm volatile ("mov %%rsp, %0" : "=r"(kernelStack));
#else
    asm volatile ("mov %%esp, %0" : "=r"(kernelStack));
#endif
    gdt_tss_set_kernel_stack(kernelStack);
    
    // Start tasking!
    interrupts_enable();
    tasking_exec();
}
