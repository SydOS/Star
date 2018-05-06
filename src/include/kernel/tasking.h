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

// Thread entry function.
typedef void (*thread_entry_func_t)(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

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
	thread_entry_func_t EntryFunc;

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
} process_t;

typedef struct {
	thread_t *CurrentThread;

	bool TaskingEnabled;
} tasking_proc_t;

extern void tasking_kill_thread(void);

extern thread_t *tasking_thread_create(process_t *process, char *name, thread_entry_func_t func, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);
extern process_t *tasking_process_create(process_t *parent, char *name, bool userMode, char *mainThreadName, thread_entry_func_t mainThreadFunc,
	uintptr_t mainThreadArg0, uintptr_t mainThreadArg1, uintptr_t mainThreadArg2);


extern thread_t *tasking_thread_create_kernel(char *name, thread_entry_func_t func, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

extern void tasking_thread_schedule_proc(thread_t *thread, uint32_t procIndex);

extern void tasking_tick(irq_regs_t* regs, uint32_t procIndex);
extern void tasking_init(void);

#endif
