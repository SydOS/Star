#include <main.h>
#include <kprint.h>
#include <kernel/memory.h>

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

void _kill() {
	// Kill a process
	if(c->pid == 1) { set_task(0); kprintf("Idle can't be killed!"); }
	kprintf("Killing process %s (%d)\n", c->name, c->pid);
	set_task(0);
	free((void *)c->stacktop);
	free(c);
	pfree(c->cr3);
	c->prev->next = c->next;
	c->next->prev = c->prev;
	set_task(1);
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
	//tasking_enable();
	//taskingEnabled = true;
	//late_init();
}

PROCESS* tasking_create_process(char* name, uint32_t addr) {
	// Set up our new process
	PROCESS* p = (PROCESS *)malloc(sizeof(PROCESS));
	memset16(p, 0, sizeof(PROCESS));
	p->name = name;
	p->pid = ++lpid;
	p->eip = addr;
	p->state = PROCESS_STATE_ALIVE;
	p->notify = __notified;
	p->esp = (uint32_t)malloc(4096);
	asm volatile("mov %%cr3, %%eax":"=a"(p->cr3));
	uint32_t* stack = (uint32_t *)(p->esp + 4096);
	p->stacktop = p->esp;
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
	p->esp = (uint32_t)stack;
	return p;
}

void tasking_init() {
	c = tasking_create_process("kernel", (uint32_t)kernel_main_thread);
	c->next = c;
	c->prev = c;
	//tasking_exec();
}