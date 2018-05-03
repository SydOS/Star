#ifndef IDT_H
#define IDT_H

#include <main.h>

// 256 interrupt entries in IDT.
#define IDT_ENTRIES 256

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
    uint16_t Limit;
    uintptr_t Base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed)) idt_ptr_t;

extern void idt_set_gate(uint8_t gate, uintptr_t base, uint16_t selector, uint8_t type, uint8_t privilege, bool present);
extern void idt_open_interrupt_gate(uint8_t gate, uintptr_t base);
extern void idt_close_interrupt_gate(uint8_t gate);
extern void idt_load();
extern void idt_init();

#endif
