#ifndef TASKING_H
#define TASKING_H

#include <arch/i386/kernel/interrupts.h>

#define PROCESS_STATE_ALIVE 0
#define PROCESS_STATE_ZOMBIE 1
#define PROCESS_STATE_DEAD 2

#define SIG_ILL 1
#define SIG_TERM 2
#define SIG_SEGV 3

typedef struct _process {
	struct _process* prev;
	char* name;
	uint32_t pid;
	uint32_t state;

	uint32_t esp;
	uint32_t *cr3;

	uint32_t stack_bottom;
	uint32_t stack_top;
	registers_t *regs;
	/* open() */
	uint16_t num_open_files;
	char **open_files;
	void (*notify)(int);
	struct _process* next;
} PROCESS;


extern void __addProcess(PROCESS* p);
extern int tasking_add_process(PROCESS* p);
extern void tasking_tick(registers_t *regs);
extern void tasking_init();

#endif
