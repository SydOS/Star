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

typedef struct {
    // FLAGS.
    bool Carry : 1;
    bool AlwaysTrue : 1;
    bool Parity : 1;
    bool Reserved1 : 1;
    bool Adjust : 1;
    bool Reserved2 : 1;
    bool Zero : 1;
    bool Sign : 1;
    bool Trap : 1;
    bool InterruptsEnabled : 1;
    bool Direction : 1;
    bool Overflow : 1;
    uint8_t PrivilegeLevel : 2;
    bool NestedTask : 1;
    bool Reserved3 : 1;

    // EFLAGS.
    bool Resume : 1;
    bool Virtual8086 : 1;
    bool AlignmentCheck : 1;
    bool VirtualInterrupt : 1;
    bool VirtualInterruptPending : 1;
    bool CPUID1 : 1;
    bool CPUID2 : 1;
    uint16_t VAD : 9;

#ifdef X86_64
    // RFLAGS.
    uint32_t Reserved4 : 32;
#endif
} __attribute__((packed)) irq_regs_flags_t;

// Defines registers for IRQ callbacks.
typedef struct {
    // Extra segments and data segment.
    uintptr_t GS, FS, ES, DS;

#ifdef X86_64
    // Extra x64 registers.
    uint64_t R8, R9, R10, R11, R12, R13, R14, R15;
#endif
    // Destination index, source index, base pointer.
    uintptr_t DI, SI, BP;

    // Base, data, counter, and accumulator registers.
    uintptr_t DX, CX, BX, AX;

    // Instruction pointer and code segment.
    uintptr_t IP, CS;

    // Flags.
    irq_regs_flags_t FLAGS;
    
    // Stack pointer and stack segment. In 32-bit these are only used when changing rings.
    uintptr_t SP;
    uintptr_t SS;   
} __attribute__((packed)) irq_regs_t;

typedef bool (*irq_handler_func_t)(irq_regs_t *regs, uint8_t irqNum);

typedef struct irq_handler_t {
    struct irq_handler_t *Next;
    irq_handler_func_t HandlerFunc;
} irq_handler_t;

extern uint8_t irqs_get_count(void);
extern bool irqs_irq_executing(void);
extern void irqs_eoi(uint8_t irq);
extern void irqs_install_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern void irqs_remove_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern bool irqs_handler_mapped(uint8_t irq, irq_handler_func_t handlerFunc);
extern void irqs_init(void);

#endif
