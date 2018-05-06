#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <main.h>
#include <kernel/interrupts/idt.h>

extern void interrupts_enable(void);
extern void interrupts_disable(void);
extern bool interrupts_enabled(void);
extern void interrupts_nmi_enable(void);
extern void interrupts_disable_enable(void);

extern void interrupts_init_ap(void);
extern void interrupts_init_bsp(void);

#endif
