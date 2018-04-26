#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <main.h>

extern void interrupts_enable(void);
extern void interrupts_disable(void);
extern bool interrupts_enabled(void);
extern void interrupts_nmi_enable(void);
extern void interrupts_disable_enable(void);
extern void interrupts_init(void);

#endif
