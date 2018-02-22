#include <main.h>
#include <kprint.h>
#include <string.h>
#include <arch/i386/kernel/idt.h>

// Create an IDT.
idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idtPtr;

// Sets an entry in the IDT.
void idt_set_gate(uint8_t gate, uint32_t base, uint16_t selector, uint8_t flags) {
    // The base address of the interrupt routine.
    idt[gate].baseLow = base & 0xFFFF;
    idt[gate].baseHigh = (base >> 16) & 0xFFFF;

    // The segment of the IDT entry.
    idt[gate].selector = selector;
    idt[gate].unused = 0;
    idt[gate].flags = flags;
}

// Initializes the IDT.
void idt_init() {
    // Set up the IDT pointer and limit.
    idtPtr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtPtr.base = (uint32_t)&idt;

    // Clear out the IDT with zeros.
    memset(&idt, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    // Load the IDT into the processor.
    asm volatile ("lidt %0" : : "g"(idtPtr));
    kprintf("IDT initialized at 0x%X!\n", &idtPtr);
}
