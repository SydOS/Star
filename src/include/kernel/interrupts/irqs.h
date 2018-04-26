#ifndef IRQS_H
#define IRQS_H

#include <main.h>

#define IRQ_OFFSET      32
#define IRQ_ISA_COUNT       16

// Common IRQs.
// https://wiki.osdev.org/Interrupts#General_IBM-PC_Compatible_Interrupt_Information
enum {
    IRQ_PIT         = 0,
    IRQ_KEYBOARD    = 1,
    IRQ_COM2        = 3,
    IRQ_COM1        = 4,
    IRQ_LPT2        = 5,
    IRQ_FLOPPY      = 6,
    IRQ_LPT1        = 7,
    IRQ_RTC         = 8,
    IRQ_ACPI        = 9,
    IRQ_MOUSE       = 12,
    IRQ_FPU         = 13,
    IRQ_PRI_ATA     = 14,
    IRQ_SEC_ATA     = 15
};

// Defines registers for IRQ callbacks.
typedef struct {
    // Extra segments and data segment.
    uintptr_t gs, fs, es, ds;

#ifdef X86_64
    // Extra x64 registers.
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
#endif
    // Destination index, source index, base pointer.
    uintptr_t di, si, bp;

#ifndef X86_64
    // Stack pointer (only in 32-bit).
    uintptr_t sp;
#endif

    // Base, data, counter, and accumulator registers.
    uintptr_t bx, dx, cx, ax;

    // Instruction pointer, code segment, and flags.
    uintptr_t ip, cs, flags;
    
#ifdef X86_64
    // Stack pointer (only in 64-bit).
    uintptr_t sp;
#else
    // Stack pointer used when changing rings (only in 32-bit).
    uintptr_t usersp;
#endif
    
    // Stack segment.
    uintptr_t ss;   
} __attribute__((packed)) irq_regs_t;

typedef bool (*irq_handler_func_t)(irq_regs_t *regs, uint8_t irqNum);

typedef struct irq_handler_t {
    struct irq_handler_t *Next;
    irq_handler_func_t HandlerFunc;
} irq_handler_t;

extern uint8_t irqs_get_count(void);
extern void irqs_eoi(uint8_t irq);
extern void irqs_install_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern void irqs_remove_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern bool irqs_handler_mapped(uint8_t irq, irq_handler_func_t handlerFunc);
extern void irqs_init(void);

#endif
