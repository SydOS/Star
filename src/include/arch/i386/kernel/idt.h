#ifndef IDT_H
#define IDT_H

#include <main.h>

// Defines an IDT entry.
struct idt_entry {
    uint16_t baseLow;   // The lower 16 bits of the address to jump to when this interrupt fires.
    uint16_t selector;  // Kernel segment selector.
    uint8_t unused;     // This must always be zero.
    uint8_t flags;      // More flags. See documentation.
    uint16_t baseHigh;  // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr {
    uint16_t limit;
    uint32_t base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

#define IDT_ENTRIES 256

extern void idt_set_gate(uint8_t gate, uint32_t base, uint16_t selector, uint8_t flags);
extern void idt_init();

#endif
