#include <main.h>
#include <kprint.h>
#include <string.h>

#include <kernel/tasking.h>
#include <kernel/memory/kheap.h>
#include <kernel/main.h>
#include <kernel/interrupts/interrupts.h>

extern void _isr_exit();

PROCESS* c = 0;
uint32_t lpid = 0;
bool taskingEnabled = false;

// -----------------------------------------------------------------------------

void schedule_noirq() {
	if(!taskingEnabled) return;
	asm volatile("int $0x2e");
	return;
}

void _kill() {
	// Kill a process
	/*if(c->pid == 1) { pit_set_task(0); kprintf("Idle can't be killed!"); }
	kprintf("Killing process %s (%d)\n", c->name, c->pid);
	pit_set_task(0);
	kheap_free((void *)c->stacktop);
	kheap_free(c);
	kheap_free(c->cr3);
	c->prev->next = c->next;
	c->next->prev = c->prev;
	pit_set_task(1);
	schedule_noirq();*/
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

/* add process but take care of others also! */
int tasking_add_process(PROCESS* p)
{
	taskingEnabled = false;
	__addProcess(p);
	taskingEnabled = true;
	return p->pid;
}

void kernel_main_thread() {
	taskingEnabled = true;
	kernel_late();
}

/* This adds a process while no others are running! */
void __addProcess(PROCESS* p)
{
	p->next = c->next;
	p->next->prev = p;
	p->prev = c;
	c->next = p;
}

PROCESS* tasking_create_process(char* name, uintptr_t addr) {
	// Allocate memory for process.
	kprintf("Allocating memory for process \"%s\"...\n", name);
	PROCESS* process = (PROCESS*)kheap_alloc(sizeof(PROCESS));
	memset(process, 0, sizeof(PROCESS));


	kprintf("Setting up process\n");
	process->name = name;
	process->pid = ++lpid;
	process->state = PROCESS_STATE_ALIVE;
	process->notify = __notified;

	kprintf("Allocating memory for stack\n");
	process->stack_bottom = (uintptr_t)kheap_alloc(4096);
	//asm volatile("mov %%cr3, %%eax":"=a"(process->cr3));

	process->stack_top = process->stack_bottom + 4096;
	memset((void*)process->stack_bottom, 0, 4096);

	process->stack_top -= sizeof(registers_t);
	process->regs = (registers_t*)process->stack_top;

	kprintf("Setting up stack...\n");
#ifdef X86_64
	process->regs->rflags = 0x00000202;
	process->regs->cs = 0x8;
	process->regs->rip = addr;
	process->regs->rbp = process->stack_top;
	process->regs->rsp = process->stack_top;
	process->regs->ds = 0x10;
	process->regs->fs = 0x10;
	process->regs->es = 0x10;
	process->regs->gs = 0x10;
#else
	process->regs->eflags = 0x00000202;
	process->regs->cs = 0x8;
	process->regs->eip = addr;
	process->regs->ebp = process->stack_top;
	process->regs->esp = process->stack_top;
	process->regs->ds = 0x10;
	process->regs->fs = 0x10;
	process->regs->es = 0x10;
	process->regs->gs = 0x10;
#endif
	return process;
}

void tasking_tick(registers_t *regs) {
	// Is tasking enabled?
	if (!taskingEnabled)
		return;

	// Save registers and move to next task.
	c->regs = regs;
	c = c->next;

	// Send EOI and change out stack.
	interrupts_eoi(0);
#ifdef X86_64
	asm volatile ("mov %0, %%rsp" : : "r"(c->regs));
#else
	asm volatile ("mov %0, %%esp" : : "r"(c->regs));
#endif
	asm volatile ("jmp _isr_exit");
}

void tasking_exec() {
	interrupts_eoi(0);
#ifdef X86_64
	asm volatile ("mov %0, %%rsp" : : "r"(c->regs));
#else
	asm volatile ("mov %0, %%esp" : : "r"(c->regs));
#endif
	asm volatile ("jmp _isr_exit");
}

void tasking_init() {
	// Start up kernel task.
	asm volatile ("cli");
	kprintf("Creating kernel task...\n");
	c = tasking_create_process("kernel", (uintptr_t)kernel_main_thread);
	c->next = c;
	c->prev = c;
	asm volatile ("sti");

	// Start tasking!
	tasking_exec();
}