#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/interrupts/idt.h>

// Create an IDT.
idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idtPtr;

// Sets an entry in the IDT.
void idt_set_gate(uint8_t gate, uintptr_t base, uint16_t selector, uint8_t flags) {
    // The base address of the interrupt routine.
    idt[gate].baseLow = base & 0xFFFF;

    // The segment of the IDT entry.
    idt[gate].selector = selector;
    idt[gate].unused = 0;
    idt[gate].flags = flags;

#ifdef X86_64
    idt[gate].baseMiddle = (base >> 16) & 0xFFFF;
    idt[gate].baseHigh = (base >> 32) & 0xFFFFFFFF;
    idt[gate].unused2 = 0;
#else
    idt[gate].baseHigh = (base >> 16) & 0xFFFF;
#endif
}

void idt_load() {
    // Load the IDT into the processor.
    asm volatile ("lidt %0" : : "g"(idtPtr));
    kprintf("IDT: Loaded at 0x%X.\n", &idtPtr);
}

// Initializes the IDT.
void idt_init() {
    kprintf("IDT: Initializing...\n");

    // Set up the IDT pointer and limit.
    idtPtr.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtPtr.base = (uintptr_t)&idt;

    // Clear out the IDT with zeros.
    memset(&idt, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    // Load the IDT.
    idt_load();
    kprintf("IDT: Initialized!\n");
}
