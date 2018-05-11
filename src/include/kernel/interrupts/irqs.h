/*
 * File: irqs.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IRQS_H
#define IRQS_H

#include <main.h>
#include <kernel/interrupts/idt.h>

#define IRQ_OFFSET      32
#define IRQ_ISA_COUNT       16

// Common IRQs.
// https://wiki.osdev.org/Interrupts#General_IBM-PC_Compatible_Interrupt_Information
enum {
    IRQ_TIMER       = 0,
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

typedef bool (*irq_handler_func_t)(irq_regs_t *regs, uint8_t irqNum, uint32_t procIndex);

typedef struct irq_handler_t {
    // Pointer to next handler.
    struct irq_handler_t *Next;

    // Handler function.
    irq_handler_func_t HandlerFunc;

    // Processor index the handler belongs to.
    uint32_t ProcessorIndex;
} irq_handler_t;

extern uint8_t irqs_get_count(void);
extern bool irqs_irq_executing(void);
extern void irqs_eoi(uint8_t irq);

extern void irqs_install_handler_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex);
extern void irqs_install_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern void irqs_remove_handler_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex);
extern void irqs_remove_handler(uint8_t irq, irq_handler_func_t handlerFunc);
extern bool irqs_handler_mapped_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex);
extern bool irqs_handler_mapped(uint8_t irq, irq_handler_func_t handlerFunc);

extern void irqs_init(idt_entry_t *idt);

#endif
