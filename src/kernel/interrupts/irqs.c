#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/interrupts/irqs.h>

#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/ioapic.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/interrupts/pic.h>
#include <kernel/memory/kheap.h>

// Common IRQ assembly handler.
extern void _irq_common(void);

// Array of IRQ handler pointers.
static uint8_t irqCount = 0;
static irq_handler *irqHandlers;

// Do we send EOIs to the LAPIC instead of the PIC?
static bool useLapic = false;

void irqs_eoi(uint8_t irqNum) {
    // Send EOI to LAPIC or PIC.
    if (useLapic)
        lapic_eoi();
    else
        pic_eoi(irqNum);
}

// Installs an IRQ handler.
void irqs_install_handler(uint8_t irq, irq_handler handler) {
    // Ensure IRQ is valid.
    if (irq >= irqCount)
        panic("IRQS: IRQ out of range.\n");

    // Add handler for IRQ to array.
    irqHandlers[irq] = handler;
    kprintf("Interrupt for IRQ%u installed!\n", irq);
}

// Removes an IRQ handler.
void irqs_remove_handler(uint8_t irq) {
    // Ensure IRQ is valid.
    if (irq >= irqCount)
        panic("IRQS: IRQ out of range.\n");

    // Null out handler for IRQ in array.
    irqHandlers[irq] = 0;
}

// Handler for IRQss.
void irqs_handler(IrqRegisters_t *regs) {
    // Get IRQ number.
    uint8_t irqNum = useLapic ? lapic_get_irq() : pic_get_irq();

    // Invoke any handler registered.
    irq_handler handler = irqHandlers[irqNum];
    if (handler)
        handler(regs, irqNum);

    // Send EOI.
    irqs_eoi(irqNum);
}

void irqs_init(void) {
    kprintf("IRQS: Intializing...\n");

    // Initialize PIC and I/O APIC.
    pic_init();
    ioapic_init();
    useLapic = false;
    irqCount = IRQ_ISA_COUNT;

    // If ACPI and the I/O APIC are supported, changeover to that.
    if (acpi_supported() && ioapic_supported()) {
        kprintf("IRQS: Using APICs for IRQs.\n");

        // Disable PIC and initialize LAPIC.
        pic_disable();
        lapic_init();

        // Change into APIC mode on ACPI.
        if (acpi_change_pic_mode(1))
            kprintf("IRQS: Now using APIC mode in ACPI.\n");
        else
            kprintf("IRQS: Couldn't change ACPI into APIC mode. The _PIC method may not exist.\n");

        // Enable all 15 ISA IRQs on the I/O APIC, except for 2.
        for (uint8_t i = 0; i < IRQ_ISA_COUNT; i++) {
            // IRQ 2 is not used.
            if (i == 2)
                continue;
            
            // Enable IRQ.
            ioapic_enable_interrupt(ioapic_remap_interrupt(i), IRQ_OFFSET + i);
        }
        useLapic = true;
        irqCount = ioapic_max_interrupts();
    }

    // Allocate space for handler array.
    irqHandlers = kheap_alloc(sizeof(irq_handler) * irqCount);
    memset(irqHandlers, 0, sizeof(irq_handler) * irqCount);

    // Open gates in IDT.
    for (uint8_t irq = 0; irq < irqCount; irq++)
        idt_open_interrupt_gate(irq + IRQ_OFFSET, _irq_common);
    kprintf("IRQS: Initialized!\n");
}
