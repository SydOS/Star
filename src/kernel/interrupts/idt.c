#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/interrupts/idt.h>
#include <kernel/gdt.h>

// Represents the IDT for the boot processor.
static idt_entry_t bspIdt[IDT_ENTRIES];

/**
 * Gets the BSP's IDT.
 */
idt_entry_t *idt_get_bsp(void) {
    return bspIdt;
}

// Sets an entry in the IDT.
void idt_set_gate(idt_entry_t *idt, uint8_t gate, uintptr_t base, uint16_t selector, uint8_t type, uint8_t privilege, bool present) {
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

void idt_open_interrupt_gate(idt_entry_t *idt, uint8_t gate, uintptr_t base) {
    // Open an interrupt gate.
    idt_set_gate(idt, gate, base, GDT_KERNEL_CODE_OFFSET, IDT_GATE_INTERRUPT_32, GDT_PRIVILEGE_KERNEL, true);
}

void idt_close_interrupt_gate(idt_entry_t *idt, uint8_t gate) {
    // Close an interrupt gate.
    idt_set_gate(idt, gate, 0, 0, 0, 0, false);
}

void idt_load(idt_entry_t *idt) {
    // Create IDT pointer.
    idt_ptr_t idtPtr = { IDT_SIZE - 1, idt};

    // Load the IDT into the processor.
    asm volatile ("lidt %0" : : "g"(idtPtr));
    kprintf("IDT: Loaded at 0x%p.\n", idt);
}

/**
 * Initializes an IDT.
 * @param idt   The IDT to initialize and load.
 */
void idt_init(idt_entry_t *idt) {
    // Clear out the IDT with zeros.
    kprintf("IDT: Initializing...\n");
    memset(idt, 0, IDT_SIZE);

    // Load the IDT.
    idt_load(idt);
    kprintf("IDT: Initialized!\n");
}

/**
 * Initializes the BSP's IDT.
 */
void idt_init_bsp(void) {
    idt_init(&bspIdt);
}
