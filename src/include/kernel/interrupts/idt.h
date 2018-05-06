/*
 * File: idt.h
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

#ifndef IDT_H
#define IDT_H

#include <main.h>

// 256 interrupt entries in IDT.
#define IDT_ENTRIES 256

// Gate types.
#define IDT_GATE_TASK_32        0x5
#define IDT_GATE_INTERRUPT_16   0x6
#define IDT_GATE_TRAP_16        0x7
#define IDT_GATE_INTERRUPT_32   0xE
#define IDT_GATE_TRAP_32        0xF

// Defines an IDT entry.
typedef struct {
    // Low 16 bits of function address.
    uint16_t BaseLow;    

    // Segment selector of function. Usually the kernel.
    uint16_t Selector;      

#ifdef X86_64
    // Interrupt stack table offset.
    uint8_t StackTableOffset : 3;

    // Unused, must be zero.
    uint8_t Unused : 5;     
#else
    // Unused, must be zero.
    uint8_t Unused;     
#endif

    // Gate type.
    uint8_t GateType : 4; 

    // False for interrupt and trap gates.
    bool StorageSegement : 1;

    // Privilege level.
    uint8_t PrivilegeLevel : 2;

    // Whether interrupt is present.
    bool Present : 1;

#ifdef X86_64
    // Middle 16 bits of function address.    
    uint16_t BaseMiddle;   

    // High 32 bits of function address.
    uint32_t BaseHigh;     

    // Unused, must be zero.
    uint32_t Unused2;      
#else
    // High 16 bits of function address.
    uint16_t BaseHigh;      
#endif
} __attribute__((packed)) idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
typedef struct {
    // Length of IDT in bytes - 1.
    uint16_t Limit;

    // Pointer to IDT.
    idt_entry_t *Table;
} __attribute__((packed)) idt_ptr_t;

#define IDT_SIZE    (sizeof(idt_entry_t) * IDT_ENTRIES)

extern idt_entry_t *idt_get_bsp(void);

extern void idt_set_gate(idt_entry_t *idt, uint8_t gate, uintptr_t base, uint16_t selector, uint8_t type, uint8_t privilege, bool present);
extern void idt_open_interrupt_gate(idt_entry_t *idt, uint8_t gate, uintptr_t base);
extern void idt_close_interrupt_gate(idt_entry_t *idt, uint8_t gate);

extern void idt_load(idt_entry_t *idt);
extern void idt_init(idt_entry_t *idt);
extern void idt_init_bsp(void);

#endif
