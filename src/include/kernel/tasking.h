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

typedef struct thread_t {
	struct thread_t *Next;
	struct thread_t *Prev;

	uint32_t ThreadId;
	char *Name;

	uint8_t Stack[THREAD_STACK_SIZE];
	uintptr_t StackPointer;
	irq_regs_t *Regs;
} thread_t;

typedef struct process_t {
	struct process_t *Next;
	struct process_t *Prev;

	char* Name;
	uint32_t ProcessId;
	bool KernelMode;

	struct thread_t *MainThread;
	struct thread_t *CurrentThread;
} process_t;

extern void tasking_kill_thread(void);
extern thread_t *tasking_thread_create(char* name, uintptr_t addr, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);
extern uint32_t tasking_thread_add(thread_t *thread, process_t *process);
extern uint32_t tasking_thread_add_kernel(thread_t *thread);

extern process_t* tasking_process_create(char* name, thread_t *mainThread, bool kernel);
extern uint32_t tasking_process_add(process_t *process);

extern void tasking_tick(irq_regs_t* regs);
extern void tasking_init(void);

#endif
