#include <main.h>
#include <kprint.h>
#include <kernel/pit.h>
#include <kernel/kheap.h>
#include <kernel/main.h>

typedef struct _process {
	struct _process* prev;
	char* name;
	uint32_t pid;
	uint32_t esp;
	uint32_t stacktop;
	uint32_t eip;
	uint32_t cr3;
	uint32_t state;
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
	if(c->pid == 1) { pit_set_task(0); kprintf("Idle can't be killed!"); }
	kprintf("Killing process %s (%d)\n", c->name, c->pid);
	pit_set_task(0);
	kheap_free((void *)c->stacktop);
	kheap_free(c);
	kheap_free(c->cr3);
	c->prev->next = c->next;
	c->next->prev = c->prev;
	pit_set_task(1);
	schedule_noirq();
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
	while (1) {  }
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
	// Set up our new process
	kprintf("Allocating memory for our process\n");
	PROCESS* p = (PROCESS *)kheap_alloc(sizeof(PROCESS));
	kprintf("memset\n");
	memset(p, 0, sizeof(PROCESS));
	kprintf("Setting up process\n");
	p->name = name;
	p->pid = ++lpid;
	p->eip = addr;
	p->state = PROCESS_STATE_ALIVE;
	p->notify = __notified;
	kprintf("Allocating memory for stack\n");
	p->esp = (uint32_t)kheap_alloc(4096);
	asm volatile("mov %%cr3, %%eax":"=a"(p->cr3));
	uint32_t* stack = (uint32_t *)(p->esp + 4096);
	p->stacktop = p->esp;
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
	*--stack = p->esp + 4096;  // ebp
	*--stack = 0x10;           // ds
	*--stack = 0x10;           // fs
	*--stack = 0x10;           // es
	*--stack = 0x10;           // gs
	// Set our new stack to the process's stack pointer and return
	kprintf("Setting stack to process\n");
	p->esp = (uint32_t)stack;
	return p;
}

void tasking_tick() {
	//asm volatile("add $0xc, %esp");
	asm volatile("push %eax");
	asm volatile("push %ebx");
	asm volatile("push %ecx");
	asm volatile("push %edx");
	asm volatile("push %esi");
	asm volatile("push %edi");
	asm volatile("push %ebp");
	asm volatile("push %ds");
	asm volatile("push %es");
	asm volatile("push %fs");
	asm volatile("push %gs");
	asm volatile("mov %%esp, %%eax":"=a"(c->esp));
	c = c->next;
	asm volatile("mov %%eax, %%cr3": :"a"(c->cr3));
	asm volatile("mov %%eax, %%esp": :"a"(c->esp));
	asm volatile("pop %gs");
	asm volatile("pop %fs");
	asm volatile("pop %es");
	asm volatile("pop %ds");
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("out %%al, %%dx": :"d"(0x20), "a"(0x20)); // send EoI to master PIC
	asm volatile("pop %edx");
	asm volatile("pop %ecx");
	asm volatile("pop %ebx");
	asm volatile("pop %eax");
	asm volatile("iret");
}

void tasking_exec()
{
	asm volatile("mov %%eax, %%esp": :"a"(c->esp));
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
	kprintf("Creating kernel task...\n");
	c = tasking_create_process("kernel", (uint32_t)kernel_main_thread);
	c->next = c;
	c->prev = c;
	kprintf("Adding second task...\n");
	__addProcess(tasking_create_process("hmmm", (uint32_t)hmmm_thread));
	kprintf("Starting tasking...\n");
	tasking_exec();
}