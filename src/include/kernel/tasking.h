#ifndef TASKING_H
#define TASKING_H

#include <kernel/interrupts/irqs.h>

#define PROCESS_STATE_ALIVE 0
#define PROCESS_STATE_ZOMBIE 1
#define PROCESS_STATE_DEAD 2

#define SIG_ILL 1
#define SIG_TERM 2
#define SIG_SEGV 3

typedef struct Process {
	struct Process* Prev;
	char* Name;
	uint32_t Pid;
	uint32_t State;

	uintptr_t Esp;
	uint32_t *Cr3;

	uintptr_t StackBottom;
	uintptr_t StackTop;
	irq_regs_t *Regs;
	/* open() */
	uint16_t num_open_files;
	char **open_files;
	void (*Notify)(int);
	struct Process* Next;
} Process;

typedef struct thread_t {
	struct thread_t *Next;
	struct thread_t *Prev;

	uint32_t ThreadId;
	char *Name;
	bool User;

	uintptr_t SP;
	uintptr_t SS;
	uintptr_t StackBottom;
	uintptr_t StackTop;
	irq_regs_t *Regs;
} thread_t;

typedef struct process_t {
	struct process_t *Next;
	struct process_t *Prev;

	char* Name;
	uint32_t ProcessId;

	struct thread_t *MainThread;
	struct thread_t *CurrentThread;
} process_t;


extern thread_t *tasking_thread_create(char *name, uintptr_t addr, uintptr_t ecx, uintptr_t edx, bool userLevel);
extern uint32_t tasking_thread_add(thread_t *thread, process_t *process);
extern uint32_t tasking_thread_add_kernel(thread_t *thread);
extern process_t* tasking_process_create(char* name, thread_t *mainThread);
extern uint32_t tasking_process_add(process_t *process);

extern int tasking_add_process(Process* newProcess);
extern Process* tasking_create_process(char* name, uintptr_t addr, uintptr_t ecx, uintptr_t edx);
extern void tasking_tick(irq_regs_t* regs);
extern void tasking_init(void);

#endif
