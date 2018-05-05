#ifndef TASKING_H
#define TASKING_H

#include <kernel/interrupts/irqs.h>

#define PROCESS_STATE_ALIVE 0
#define PROCESS_STATE_ZOMBIE 1
#define PROCESS_STATE_DEAD 2

#define SIG_ILL 1
#define SIG_TERM 2
#define SIG_SEGV 3

#define THREAD_STACK_SIZE	4096

// Used for circular dependencies.
struct thread_t;
struct process_t;

typedef struct thread_t {
	// Relationship to other threads and parent process.
	struct thread_t *Next;
	struct thread_t *Prev;
	struct process_t *Parent;

	// Thread ID info.
	char *Name;
	uint32_t ThreadId;

	// Stack.
	uint64_t StackPage;
	uintptr_t StackPointer;

	// Scheduling relationship to other threads.
	struct thread_t *SchedNext;
	struct thread_t *SchedPrev;
} thread_t;

typedef struct process_t {
	// Relationship to other processes.
	struct process_t *Next;
	struct process_t *Prev;
	struct process_t *Parent;

	char* Name;
	uint32_t ProcessId;
	uintptr_t PagingTablePhys;
	bool UserMode;

	thread_t *MainThread;



	thread_t *CurrentThread;
} process_t;

typedef struct {
	thread_t *CurrentThread;

	bool TaskingEnabled;
} tasking_proc_t;

extern void tasking_kill_thread(void);
extern thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
extern uint32_t tasking_thread_add(thread_t *thread, process_t *process);
extern uint32_t tasking_thread_add_kernel(thread_t *thread);

extern process_t* tasking_process_create(char* name, thread_t *mainThread, bool kernel);
extern uint32_t tasking_process_add(process_t *process);

extern void tasking_tick(irq_regs_t* regs, uint32_t procIndex);
extern void tasking_init(void);

#endif
