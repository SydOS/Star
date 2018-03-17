#include <main.h>
#include <kprint.h>
#include <kernel/interrupts/interrupts.h>

#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/ioapic.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/interrupts/pic.h>

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

// Array of ISR handler pointers.
static isr_handler isrHandlers[ISR_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// Exception messages array.
static char *exception_messages[] = {
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

// Do we send EOIs to the LAPIC instead of the PIC?
static bool useLapic = false;

void interrupts_eoi(uint32_t intNo) {
    // Send EOI to LAPIC or PIC.
    if (useLapic)
        lapic_eoi();
    else
        pic_eoi(intNo);
}

// Installs an ISR handler.
void interrupts_isr_install_handler(uint8_t isr, isr_handler handler) {
    // Add handler for ISR to array.
    isrHandlers[isr] = handler;
    kprintf("Interrupt for ISR%u installed!\n", isr);
}

// Removes an ISR handler.
void interrupts_isr_remove_handler(uint8_t isr) {
    // Null out handler for ISR in array.
    isrHandlers[isr] = 0;
}

// Installs an IRQ handler.
void interrupts_irq_install_handler(uint8_t irq, isr_handler handler) {
    // Add handler for IRQ to array.
    isrHandlers[irq + IRQ_OFFSET] = handler;
    kprintf("Interrupt for IRQ%u installed!\n", irq);
}

// Removes an IRQ handler.
void interrupts_irq_remove_handler(uint8_t irq) {
    // Null out handler for IRQ in array.
    isrHandlers[irq + IRQ_OFFSET] = 0;
}

// Handler for ISRs.
void interrupts_isr_handler(registers_t *regs) {
    uint32_t intNum, errorCode;
    // Blank handler pointer.
    isr_handler handler;

    intNum = regs->intNum;
    errorCode = regs->errorCode;

    // Invoke any handler registered.
    handler = isrHandlers[intNum];
    if (handler) {
        handler(regs);
    }
    else {
        // If we have an exception, print default message.
        if (intNum < IRQ_OFFSET) {
            panic("CPU%d: Exception: %s\n", lapic_id(), exception_messages[intNum]);
        }
    }

    // Send EOI if IRQ.
    if (intNum >= IRQ_OFFSET)
        interrupts_eoi(intNum - IRQ_OFFSET);
}

inline void interrupts_enable() {
    asm volatile ("sti");
}

inline void interrupts_disable() {
    asm volatile ("cli");
}

// Initializes interrupts.
void interrupts_init() {
    kprintf("INTERRUPTS: Initializing...\n");

    // Initialize PIC and I/O APIC.
    pic_init();
    ioapic_init();
    useLapic = false;

    if (acpi_supported() && ioapic_supported()) {
        kprintf("INTERRUPTS: Using APICs for interrupts.\n");

        // Disable PIC and initialize LAPIC.
        pic_disable();
        lapic_init();

        // Enable all 15 IRQs on the I/O APIC, except for 2.
        for (uint8_t i = 0; i < IRQ_COUNT; i++) {
            // IRQ 2 is not used.
            if (i == 2)
                continue;
            
            // Enable IRQ.
            ioapic_enable_interrupt(ioapic_remap_interrupt(i), IRQ_OFFSET + i);
        }
        useLapic = true;
    }

    // Add each of the 32 exception ISRs to the IDT.
    kprintf("INTERRUPTS: Mapping exceptions...\n");
    idt_set_gate(0, (uintptr_t)_isr0, 0x08, 0x8E);
    idt_set_gate(1, (uintptr_t)_isr1, 0x08, 0x8E);
    idt_set_gate(2, (uintptr_t)_isr2, 0x08, 0x8E);
    idt_set_gate(3, (uintptr_t)_isr3, 0x08, 0x8E);
    idt_set_gate(4, (uintptr_t)_isr4, 0x08, 0x8E);
    idt_set_gate(5, (uintptr_t)_isr5, 0x08, 0x8E);
    idt_set_gate(6, (uintptr_t)_isr6, 0x08, 0x8E);
    idt_set_gate(7, (uintptr_t)_isr7, 0x08, 0x8E);

    idt_set_gate(8, (uintptr_t)_isr8, 0x08, 0x8E);
    idt_set_gate(9, (uintptr_t)_isr9, 0x08, 0x8E);
    idt_set_gate(10, (uintptr_t)_isr10, 0x08, 0x8E);
    idt_set_gate(11, (uintptr_t)_isr11, 0x08, 0x8E);
    idt_set_gate(12, (uintptr_t)_isr12, 0x08, 0x8E);
    idt_set_gate(13, (uintptr_t)_isr13, 0x08, 0x8E);
    idt_set_gate(14, (uintptr_t)_isr14, 0x08, 0x8E);
    idt_set_gate(15, (uintptr_t)_isr15, 0x08, 0x8E);

    idt_set_gate(16, (uintptr_t)_isr16, 0x08, 0x8E);
    idt_set_gate(17, (uintptr_t)_isr17, 0x08, 0x8E);
    idt_set_gate(18, (uintptr_t)_isr18, 0x08, 0x8E);
    idt_set_gate(19, (uintptr_t)_isr19, 0x08, 0x8E);
    idt_set_gate(20, (uintptr_t)_isr20, 0x08, 0x8E);
    idt_set_gate(21, (uintptr_t)_isr21, 0x08, 0x8E);
    idt_set_gate(22, (uintptr_t)_isr22, 0x08, 0x8E);

    idt_set_gate(23, (uintptr_t)_isr23, 0x08, 0x8E);
    idt_set_gate(24, (uintptr_t)_isr24, 0x08, 0x8E);
    idt_set_gate(25, (uintptr_t)_isr25, 0x08, 0x8E);
    idt_set_gate(26, (uintptr_t)_isr26, 0x08, 0x8E);
    idt_set_gate(27, (uintptr_t)_isr27, 0x08, 0x8E);
    idt_set_gate(28, (uintptr_t)_isr28, 0x08, 0x8E);
    idt_set_gate(29, (uintptr_t)_isr29, 0x08, 0x8E);
    idt_set_gate(30, (uintptr_t)_isr30, 0x08, 0x8E);
    idt_set_gate(31, (uintptr_t)_isr31, 0x08, 0x8E);

    // Add the 16 IRQ ISRs to the IDT.
    kprintf("INTERRUPTS: Mapping IRQs...\n");
    idt_set_gate(32, (uintptr_t)_irq0, 0x08, 0x8E);
    idt_set_gate(33, (uintptr_t)_irq1, 0x08, 0x8E);
    idt_set_gate(34, (uintptr_t)_irq2, 0x08, 0x8E);
    idt_set_gate(35, (uintptr_t)_irq3, 0x08, 0x8E);
    idt_set_gate(36, (uintptr_t)_irq4, 0x08, 0x8E);
    idt_set_gate(37, (uintptr_t)_irq5, 0x08, 0x8E);
    idt_set_gate(38, (uintptr_t)_irq6, 0x08, 0x8E);
    idt_set_gate(39, (uintptr_t)_irq7, 0x08, 0x8E);
    idt_set_gate(40, (uintptr_t)_irq8, 0x08, 0x8E);
    idt_set_gate(41, (uintptr_t)_irq9, 0x08, 0x8E);
    idt_set_gate(42, (uintptr_t)_irq10, 0x08, 0x8E);
    idt_set_gate(43, (uintptr_t)_irq11, 0x08, 0x8E);
    idt_set_gate(44, (uintptr_t)_irq12, 0x08, 0x8E);
    idt_set_gate(45, (uintptr_t)_irq13, 0x08, 0x8E);
    idt_set_gate(46, (uintptr_t)_irq14, 0x08, 0x8E);
    idt_set_gate(47, (uintptr_t)_irq15, 0x08, 0x8E);

    kprintf("INTERRUPTS: Initialized!\n");
}
