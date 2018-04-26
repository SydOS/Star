#ifndef IDT_H
#define IDT_H

#include <main.h>

// 256 interrupt entries in IDT.
#define IDT_ENTRIES 256

// Defines an IDT entry.
typedef struct {
    uint16_t BaseLow;       // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t Selector;      // Kernel segment selector.
    uint8_t Unused;         // This must always be zero.
    uint8_t Flags;          // More flags. See documentation.

#ifdef X86_64
    uint16_t BaseMiddle;    // The middle 16 bits of the address to jump to.
    uint32_t BaseHigh;      // The upper 32 bits of the address to jump to.
    uint32_t Unused2;       // Must be zero.
#else
    uint16_t BaseHigh;      // The upper 16 bits of the address to jump to.
#endif
} __attribute__((packed)) idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
typedef struct {
    uint16_t Limit;
    uintptr_t Base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed)) idt_ptr_t;

extern void idt_set_gate(uint8_t gate, uintptr_t base, uint16_t selector, uint8_t flags);
extern void idt_open_interrupt_gate(uint8_t gate, uintptr_t base);
extern void idt_close_interrupt_gate(uint8_t gate);
extern void idt_load();
extern void idt_init();

#endif
