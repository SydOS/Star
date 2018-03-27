#include <main.h>
#include <kprint.h>
#include <string.h>

#include <kernel/tasking.h>
#include <kernel/memory/kheap.h>
#include <kernel/main.h>
#include <kernel/interrupts/interrupts.h>

extern void _isr_exit();

// Current process.
Process* currentProcess = 0;
uint32_t lpid = 0;
bool taskingEnabled = false;

void _kill() {
    // Pause tasking.
    tasking_freeze();

    // Remove task.
    currentProcess->Prev->Next = currentProcess->Next;
    currentProcess->Next->Prev = currentProcess->Prev;

    // Free memory.
    kheap_free((void *)currentProcess->StackBottom);
    kheap_free(currentProcess);

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

inline void tasking_freeze() {
    taskingEnabled = false;
}

inline void tasking_unfreeze() {
    taskingEnabled = true;
}

int tasking_add_process(Process* newProcess) {
    // Pause tasking.
    tasking_freeze();

    // Add process into mix.
    newProcess->Next = currentProcess->Next;
    newProcess->Next->Prev = newProcess;
    newProcess->Prev = currentProcess;
    currentProcess->Next = newProcess;

    // Resume tasking.
    tasking_unfreeze();

    // Return the PID.
    return newProcess->Pid;
}

void kernel_main_thread(void) {
    tasking_unfreeze();
    kernel_late();
}

Process* tasking_create_process(char* name, uintptr_t addr, uintptr_t ecx, uintptr_t edx) {
    // Allocate memory for process.
    Process* process = (Process*)kheap_alloc(sizeof(Process));
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
    process->StackTop -= sizeof(registers_t);
    process->Regs = (registers_t*)process->StackTop;
#ifdef X86_64
    process->Regs->rflags = 0x00000202;
    process->Regs->rip = addr;
    process->Regs->rbp = process->StackTop;
    process->Regs->rsp = process->StackTop;
    process->Regs->rcx = ecx;	
    process->Regs->rdx = edx;	
#else
    process->Regs->eflags = 0x00000202;
    process->Regs->eip = addr;
    process->Regs->ebp = stackTop;
    process->Regs->esp = process->StackTop;
    process->Regs->ecx = ecx;	
    process->Regs->edx = edx;	
#endif

    process->Regs->cs = 0x8;
    process->Regs->ds = 0x10;
    process->Regs->fs = 0x10;
    process->Regs->es = 0x10;
    process->Regs->gs = 0x10;
    return process;
}

static void tasking_exec() {
    // Send EOI and change out stacks.
    interrupts_eoi(0);
#ifdef X86_64
    asm volatile ("mov %0, %%rsp" : : "r"(currentProcess->Regs));
#else
    asm volatile ("mov %0, %%esp" : : "r"(currentProcess->Regs));
#endif
    asm volatile ("jmp _isr_exit");
}

void tasking_tick(registers_t *regs) {
    // Is tasking enabled?
    if (!taskingEnabled)
        return;

    // Save registers and move to next task.
    currentProcess->Regs = regs;
    currentProcess = currentProcess->Next;

    // Jump to next task.
    tasking_exec();
}

void tasking_init() {
    // Set up kernel task.
    asm volatile ("cli");
    kprintf("Creating kernel task...\n");
    currentProcess = tasking_create_process("kernel", (uintptr_t)kernel_main_thread, 0, 0);
    currentProcess->Next = currentProcess;
    currentProcess->Prev = currentProcess;
    asm volatile ("sti");

    // Start tasking!
    tasking_exec();
}