#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <main.h>

static inline void interrupts_enable(void) {
    asm volatile ("sti");
}

static inline void interrupts_disable(void) {
    asm volatile ("cli");
}

extern void interrupts_nmi_enable(void);
extern void interrupts_disable_enable(void);
extern void interrupts_init(void);

#endif
