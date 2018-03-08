#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <main.h>

#define IRQ_OFFSET      32
#define IRQ_COUNT       16
#define ISR_COUNT       (IRQ_OFFSET+IRQ_COUNT)

// ISR exceptions.
// https://wiki.osdev.org/Exceptions
enum {
    ISR_EXCEPTION_DIVIDE_BY_ZERO            = 0,
    ISR_EXCEPTION_DEBUG                     = 1,
    ISR_EXCEPTION_NON_MASKABLE_INTERRUPT    = 2,
    ISR_EXCEPTION_BREAKPOINT                = 3,
    ISR_EXCEPTION_OVERFLOW                  = 4,
    ISR_EXCEPTION_BOUND_RANGE_EXCEEDED      = 5,
    ISR_EXCEPTION_INVALID_OPCODE            = 6,
    ISR_EXCEPTION_DEVICE_NOT_AVAILABLE      = 7,
    ISR_EXCEPTION_DOUBLE_FAULT              = 8,
    ISR_EXCEPTION_COPROCESSOR_SEG_OVERRUN   = 9,
    ISR_EXCEPTION_INVALID_TSS               = 10,
    ISR_EXCEPTION_SEGMENT_NOT_PRESENT       = 11,
    ISR_EXCEPTION_STACK_SEGMENT_FAULT       = 12,
    ISR_EXCEPTION_GENERAL_PROTECTION_FAULT  = 13,
    ISR_EXCEPTION_PAGE_FAULT                = 14,
    ISR_EXCEPTION_X87_FLOAT_POINT           = 16,
    ISR_EXCEPTION_ALIGNMENT_CHECK           = 17,
    ISR_EXCEPTION_MACHINE_CHECK             = 18,
    ISR_EXCEPTION_SIMD_FLOAT_POINT          = 19,
    ISR_EXCEPTION_VIRTUALIZATION            = 20,
    ISR_EXCEPTION_SECURITY                  = 30
};

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
    IRQ_MOUSE       = 12,
    IRQ_FPU         = 13,
    IRQ_PRI_ATA     = 14,
    IRQ_SEC_ATA     = 15
};

// Defines registers for ISR callbacks.
struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t intNum, errorCode;
    uint32_t eip, cs, eflags, useresp, ss;    
};
typedef struct registers registers_t;

typedef void (*isr_handler)(registers_t *regs);

extern void interrupts_isr_install_handler(uint8_t isr, isr_handler handler);
extern void interrupts_isr_remove_handler(uint8_t isr);
extern void interrupts_irq_install_handler(uint8_t irq, isr_handler handler);
extern void interrupts_irq_remove_handler(uint8_t irq);
extern void interrupts_init();

#endif
