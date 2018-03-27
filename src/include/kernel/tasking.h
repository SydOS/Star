#ifndef TASKING_H
#define TASKING_H

#include <kernel/interrupts/interrupts.h>

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
	registers_t *Regs;
	/* open() */
	uint16_t num_open_files;
	char **open_files;
	void (*Notify)(int);
	struct Process* Next;
} Process;


extern int tasking_add_process(Process* p);
extern Process* tasking_create_process(char* name, uintptr_t addr);
extern void tasking_tick(registers_t *regs);
extern void tasking_init();

#endif
