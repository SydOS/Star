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
	struct thread_t *Next;
	struct thread_t *Prev;

	struct process_t *Parent;

	char *Name;
	uint32_t ThreadId;

	uint64_t StackPage;
	uintptr_t StackPointer;
} thread_t;

typedef struct process_t {
	struct process_t *Next;
	struct process_t *Prev;
	struct process_t *Parent;

	char* Name;
	uint32_t ProcessId;
	uintptr_t PagingTablePhys;

	thread_t *MainThread;
	thread_t *CurrentThread;
} process_t;

typedef struct {
	thread_t *Threads;
	thread_t *CurrentThread;

	bool Started;
} tasking_proc_t;

extern void tasking_kill_thread(void);
extern thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
extern uint32_t tasking_thread_add(thread_t *thread, process_t *process);
extern uint32_t tasking_thread_add_kernel(thread_t *thread);

extern process_t* tasking_process_create(char* name, thread_t *mainThread, bool kernel);
extern uint32_t tasking_process_add(process_t *process);

extern void tasking_tick(irq_regs_t* regs);
extern void tasking_init(void);

#endif
