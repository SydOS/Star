#include <main.h>
#include <kprint.h>
#include <kernel/pit.h>
#include <kernel/kheap.h>
#include <kernel/main.h>
#include <arch/i386/kernel/interrupts.h>
#include <driver/speaker.h>
#include <driver/vga.h>

typedef struct _process {
	struct _process* prev;
	char* name;
	uint32_t pid;
	uint32_t esp;
	uint32_t cr3;
	uint32_t state;
	registers_t regs;
	/* open() */
	uint16_t num_open_files;
	char **open_files;
	void (*notify)(int);
	struct _process* next;
} PROCESS;

#define PROCESS_STATE_ALIVE 0
#define PROCESS_STATE_ZOMBIE 1
#define PROCESS_STATE_DEAD 2

#define SIG_ILL 1
#define SIG_TERM 2
#define SIG_SEGV 3

// -----------------------------------------------------------------------------

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
		case SIG_SEGV:
			kprintf("Received SIGSEGV, terminating!\n");
			_kill();
		default:
			kprintf("Received unknown SIG!\n");
			return;
	}
}

// -----------------------------------------------------------------------------

void kernel_main_thread() {
	pit_enable_tasking();
	taskingEnabled = true;
	kernel_late();
}

void hmmm_thread() {
	kprintf("hmm");
	uint32_t t = 0;
	while (1) { 
		t++;
		kprintf("hmm\n");
		sleep(1000);
		//speaker_play_tone(2000, 500);
		//kprintf("hmm\n");
	 }
}

/* This adds a process while no others are running! */
void __addProcess(PROCESS* p)
{
	p->next = c->next;
	p->next->prev = p;
	p->prev = c;
	c->next = p;
}

PROCESS* tasking_create_process(char* name, uint32_t addr) {
	// Allocate memory for process.
	kprintf("Allocating memory for process \"%s\"...\n", name);
	PROCESS* process = (PROCESS*)kheap_alloc(sizeof(PROCESS));
	memset(process, 0, sizeof(PROCESS));


	kprintf("Setting up process\n");
	process->name = name;
	process->pid = ++lpid;
	process->regs.eip = addr;
	process->state = PROCESS_STATE_ALIVE;
	process->notify = __notified;

	kprintf("Allocating memory for stack\n");
	process->regs.ebp = (uint32_t)kheap_alloc(4096);
	asm volatile("mov %%cr3, %%eax":"=a"(process->cr3));
	uint32_t* stack = (uint32_t *)(process->regs.ebp + 4096);
	kprintf("Setting up stack\n");
	// Set up our stack
	*--stack = 0x00000202;     // eflags
	*--stack = 0x8;            // cs
	*--stack = (uint32_t)addr; // eip
	*--stack = 0;              // eax
	*--stack = 0;              // ebx
	*--stack = 0;              // ecx
	*--stack = 0;              // edx
	*--stack = 0;              // esi
	*--stack = 0;              // edi
	*--stack = process->regs.ebp + 4096;  // ebp
	*--stack = 0x10;           // ds
	*--stack = 0x10;           // fs
	*--stack = 0x10;           // es
	*--stack = 0x10;           // gs

	process->regs.gs = 0x10;
	process->regs.es = 0x10;
	process->regs.fs = 0x10;
	process->regs.ds = 0x10;
	process->regs.cs = 0x08;
	process->regs.eflags = 0x00000202;

	// Set our new stack to the process's stack pointer and return
	kprintf("Setting stack to process\n");
	process->regs.esp = (uint32_t)stack;
	return process;
}

volatile void tasking_tick(registers_t *regs) {
	vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
	//kprintf("ESI: 0x%X, EDI: 0x%X, EBP: 0x%X, ESP: 0x%X\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    kprintf("EIP: 0x%X, EFLAGS: 0x%X\n", regs->eip, regs->eflags);
	kprintf("tick from %s to %s!\n", c->name, c->next->name);
	//
	//asm volatile("add $0xc, %esp");
	//asm volatile("mov %%esp, %%eax":"=a"(c->esp));

	

	memcpy(&c->regs, regs, sizeof(registers_t));
	/*c->regs.gs = regs->gs;
	c->regs.fs = regs->fs;
	c->regs.es = regs->es;
	c->regs.ds = regs->ds;
	c->regs.edi = regs->edi;
	c->regs.esi = regs->esi;
	c->regs.ebp = regs->ebp;
	c->regs.esp = regs->esp;
	c->regs.ebx = regs->ebx;
	c->regs.edx = regs->edx;
	c->regs.ecx = regs->ecx;
	c->regs.eax = regs->eax;
	c->regs.eip = regs->eip;
	c->regs.ss = regs->ss;
	c->regs.cs = regs->cs;
	c->regs.eflags = regs->eflags;
	c->regs.useresp = regs->useresp;*/



	c = c->next;
	//asm volatile("mov %%eax, %%cr3": :"a"(c->cr3));
	//asm volatile("mov %%eax, %%esp": :"a"(c->esp));
	memcpy(regs, &c->regs, sizeof(registers_t));

	/*regs->gs = c->regs.gs;
	regs->fs = c->regs.fs;
	regs->es = c->regs.es;
	regs->ds = c->regs.ds;
	regs->edi = c->regs.edi;
	regs->esi = c->regs.esi;
	regs->ebp = c->regs.ebp;
	regs->esp = c->regs.esp;
	regs->ebx = c->regs.ebx;
	regs->edx = c->regs.edx;
	regs->ecx = c->regs.ecx;
	regs->eax = c->regs.eax;
	regs->eip = c->regs.eip;
	regs->ss = c->regs.ss;
	regs->cs = c->regs.cs;
	regs->eflags = c->regs.eflags;
	regs->useresp = c->regs.useresp;*/

	//kprintf("ESI: 0x%X, EDI: 0x%X, EBP: 0x%X, ESP: 0x%X\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    kprintf("EIP: 0x%X, EFLAGS: 0x%X\n", regs->eip, regs->eflags);
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void tasking_exec()
{
	asm volatile("mov %%eax, %%esp": :"a"(c->regs.esp));
	asm volatile("pop %gs");
	asm volatile("pop %fs");
	asm volatile("pop %es");
	asm volatile("pop %ds");
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %edx");
	asm volatile("pop %ecx");
	asm volatile("pop %ebx");
	asm volatile("pop %eax");
	asm volatile("iret");
}

void tasking_init() {
	asm volatile ("cli");
	kprintf("Creating kernel task...\n");
	c = tasking_create_process("kernel", (uint32_t)kernel_main_thread);
	c->next = c;
	c->prev = c;
	kprintf("Adding second task...\n");
	__addProcess(tasking_create_process("hmmm", (uint32_t)hmmm_thread));
	kprintf("Starting tasking...\n");

	asm volatile ("sti");
	tasking_exec();
}