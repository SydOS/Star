#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/interrupts/idt.h>
#include <kernel/gdt.h>

// Create an IDT.
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idtPtr;

// Sets an entry in the IDT.
void idt_set_gate(uint8_t gate, uintptr_t base, uint16_t selector, uint8_t type, uint8_t privilege, bool present) {
    // Set low 16 bits of function address.
    idt[gate].BaseLow = base & 0xFFFF;

    // Set options.
    idt[gate].Selector = selector;
    idt[gate].Unused = 0;
    idt[gate].GateType = type;
    idt[gate].StorageSegement = false;
    idt[gate].PrivilegeLevel = privilege;
    idt[gate].Present = present;

#ifdef X86_64
    idt[gate].StackTableOffset = 0;
    idt[gate].BaseMiddle = (base >> 16) & 0xFFFF;
    idt[gate].BaseHigh = (base >> 32) & 0xFFFFFFFF;
    idt[gate].Unused2 = 0;
#else
    idt[gate].BaseHigh = (base >> 16) & 0xFFFF;
#endif
}

void idt_open_interrupt_gate(uint8_t gate, uintptr_t base) {
    // Open an interrupt gate.
    idt_set_gate(gate, base, GDT_KERNEL_CODE_OFFSET, IDT_GATE_INTERRUPT_32, GDT_PRIVILEGE_KERNEL, true);
}

void idt_close_interrupt_gate(uint8_t gate) {
    // Close an interrupt gate.
    idt_set_gate(gate, 0, 0, 0, 0, false);
}

void idt_load() {
    // Load the IDT into the processor.
    asm volatile ("lidt %0" : : "g"(idtPtr));
    kprintf("IDT: Loaded at 0x%p.\n", &idtPtr);
}

// Initializes the IDT.
void idt_init() {
    kprintf("IDT: Initializing...\n");

    // Set up the IDT pointer and limit.
    idtPtr.Limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idtPtr.Base = (uintptr_t)&idt;

    // Clear out the IDT with zeros.
    memset(&idt, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    // Load the IDT.
    idt_load();
    kprintf("IDT: Initialized!\n");
}
