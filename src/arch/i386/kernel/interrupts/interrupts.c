#include <main.h>
#include <kprint.h>
#include <arch/i386/kernel/idt.h>
#include <arch/i386/kernel/interrupts.h>
#include <arch/i386/kernel/pic.h>
#include <arch/i386/kernel/lapic.h>

// Functions defined by Intel for service extensions.
extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();

// Functions defined for IRQs.
extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();

// Array of IRQ handler pointers.
void* irqHandlers[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// Exception messages array.
char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Installs an IRQ handler.
void interrupts_irq_install_handler(uint8_t irq, void (*handler)(registers_t *regs)) {
    // Add handler for IRQ to array.
    irqHandlers[irq] = handler;
    kprintf("Interrupt for IRQ%u installed!\n", irq);
}

// Removes an IRQ handler.
void interrupts_irq_remove_handler(uint8_t irq) {
    // Null out handler for IRQ in array.
    irqHandlers[irq] = 0;
}

// Handler for exception ISRs.
void interrupts_fault_handler(registers_t *regs) {
    if(regs->intNum < 32)
        panic("Exception: %s\n", exception_messages[regs->intNum]); // TODO: only panic with actual aborts,
                                                                    // as faults are command and shouldn't panic.
}

// Handler for IRQ ISRs.
void interrupts_irq_handler(registers_t *regs) {
    // Blank handler pointer.
    void (*handler)(struct regs *r);

    // Invoke any handler registered.
    handler = irqHandlers[regs->intNum - 32];
    if (handler)
        handler(regs);

    // Send EOI to PIC.
    pic_eoi(regs->intNum);
}

// Initializes interrupts.
void interrupts_init() {
    // Check if an APIC is present.
    if (lapic_supported())
        kprintf("APIC supported!\n");

    // Enable PIC.
    pic_init();

    // Add each of the 32 exception ISRs to the IDT.
    idt_set_gate(0, (uint32_t)_isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)_isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)_isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)_isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)_isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)_isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)_isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)_isr7, 0x08, 0x8E);

    idt_set_gate(8, (uint32_t)_isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)_isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)_isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)_isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)_isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)_isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)_isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)_isr15, 0x08, 0x8E);

    idt_set_gate(16, (uint32_t)_isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)_isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)_isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)_isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)_isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)_isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)_isr22, 0x08, 0x8E);

    idt_set_gate(23, (uint32_t)_isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)_isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)_isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)_isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)_isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)_isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)_isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)_isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)_isr31, 0x08, 0x8E);

    // Add the 16 IRQ ISRs to the IDT.
    idt_set_gate(32, (uint32_t)_irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)_irq1, 0x08, 0x8E);
    idt_set_gate(34, (uint32_t)_irq2, 0x08, 0x8E);
    idt_set_gate(35, (uint32_t)_irq3, 0x08, 0x8E);
    idt_set_gate(36, (uint32_t)_irq4, 0x08, 0x8E);
    idt_set_gate(37, (uint32_t)_irq5, 0x08, 0x8E);
    idt_set_gate(38, (uint32_t)_irq6, 0x08, 0x8E);
    idt_set_gate(39, (uint32_t)_irq7, 0x08, 0x8E);
    idt_set_gate(40, (uint32_t)_irq8, 0x08, 0x8E);
    idt_set_gate(41, (uint32_t)_irq9, 0x08, 0x8E);
    idt_set_gate(42, (uint32_t)_irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)_irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)_irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)_irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)_irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)_irq15, 0x08, 0x8E);

    kprintf("Interrupts initialized!\n");
}
