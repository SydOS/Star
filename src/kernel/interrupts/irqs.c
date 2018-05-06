#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/interrupts/irqs.h>

#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/ioapic.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/interrupts/pic.h>
#include <kernel/interrupts/smp.h>
#include <kernel/memory/kheap.h>

// Common IRQ assembly handler.
extern void _irq_common(void);

// Array of IRQ handler pointers.
static uint8_t irqCount = 0;
static irq_handler_t **irqHandlers;

// Do we send EOIs to the LAPIC instead of the PIC?
static bool useLapic = false;

uint8_t irqs_get_count(void) {
    return irqCount;
}

static bool irqExecuting = false;
bool irqs_irq_executing(void) {
    return irqExecuting;
}

void irqs_eoi(uint8_t irq) {
    // Send EOI to LAPIC or PIC.
    if (useLapic)
        lapic_eoi();
    else
        pic_eoi(irq);
}

void irqs_install_handler_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex) {
    // Ensure IRQ is valid.
    if (irq >= irqCount)
        panic("IRQS: IRQ out of range.\n");

    // Create handler object.
    irq_handler_t *handler = kheap_alloc(sizeof(irq_handler_t));
    memset(handler, 0, sizeof(irq_handler_t));

    // Populate handler object.
    handler->Next = NULL;
    handler->HandlerFunc = handlerFunc;
    handler->ProcessorIndex = procIndex;

    // Add handler to end of list.
    if (irqHandlers[irq] != NULL) {
        irq_handler_t *currHandler = irqHandlers[irq];
        while (currHandler->Next != NULL)
            currHandler = currHandler->Next;
        currHandler->Next = handler;
    }
    else
        irqHandlers[irq] = handler;
    kprintf("IRQS: Handler 0x%p for IRQ%u installed!\n", handlerFunc, irq);
}

// Installs an IRQ handler.
void irqs_install_handler(uint8_t irq, irq_handler_func_t handlerFunc) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t index = (proc != NULL) ? proc->Index : 0;

    // Add handler.
    irqs_install_handler_proc(irq, handlerFunc, index);
}

void irqs_remove_handler_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex) {
    // Ensure IRQ is valid.
    if (irq >= irqCount)
        panic("IRQS: IRQ out of range.\n");

    // Try to find handler function.
    irq_handler_t *prevHandler = NULL;
    irq_handler_t *handler = irqHandlers[irq];
    while (handler != NULL) {
        if (handler->HandlerFunc == handlerFunc && handler->ProcessorIndex == procIndex)
            break;
        prevHandler = handler;
        handler = handler->Next;
    }

    // If handler is still NULL, we couldn't find the specified handler function.
    if (handler == NULL) {
        kprintf("IRQS: Unable to find and remove handler 0x%p for IRQ%u!\n", handlerFunc, irq);
        return;
    }

    // Remove handler.
    if (prevHandler != NULL)
        prevHandler->Next = handler->Next;
    else
        irqHandlers[irq] = handler->Next;
    kheap_free(handler);
    kprintf("IRQS: Handler 0x%p for IRQ%u removed!\n", handlerFunc, irq);
}

// Removes an IRQ handler.
void irqs_remove_handler(uint8_t irq, irq_handler_func_t handlerFunc) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t index = (proc != NULL) ? proc->Index : 0;

    // Remove handler.
    return irqs_remove_handler_proc(irq, handlerFunc, index);
}

bool irqs_handler_mapped_proc(uint8_t irq, irq_handler_func_t handlerFunc, uint32_t procIndex) {
    // Ensure IRQ is valid.
    if (irq >= irqCount)
        panic("IRQS: IRQ out of range.\n");

    // Try to find IRQ handler.
    irq_handler_t *handler = irqHandlers[irq];
    while (handler != NULL) {
        if (handler->HandlerFunc == handlerFunc && handler->ProcessorIndex == procIndex)
            return true;
        handler = handler->Next;
    }
    return false;
}

bool irqs_handler_mapped(uint8_t irq, irq_handler_func_t handlerFunc) {
    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t index = (proc != NULL) ? proc->Index : 0;

    // Determine if handler is mapped.
    return irqs_handler_mapped_proc(irq, handlerFunc, index);
}

// Handler for IRQss.
void irqs_handler(irq_regs_t *regs) {
    // Get IRQ number.
    irqExecuting = true;
    uint8_t irq = useLapic ? lapic_get_irq() : pic_get_irq();

    // Get processor we are running on.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    uint32_t procIndex = (proc != NULL) ? proc->Index : 0;

    // Ensure IRQ is within range.
    if (irq < irqCount) {
        // Invoke registered handlers.
        irq_handler_t *handler = irqHandlers[irq];
        while (handler != NULL) {
            if (handler->HandlerFunc != NULL && handler->ProcessorIndex == procIndex) {
                if (handler->HandlerFunc(regs, irq, procIndex))
                    break;
            }
            handler = handler->Next;
        }
    }

    // Send EOI.
    irqs_eoi(irq);
    irqExecuting = false;
}

void irqs_init(idt_entry_t *idt) {
    kprintf("IRQS: Intializing...\n");

    // Initialize PIC and I/O APIC.
    pic_init();
    ioapic_init();
    useLapic = false;
    irqCount = IRQ_ISA_COUNT;

    // If the I/O APIC is supported, changeover to that.
    if (ioapic_supported()) {
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
    kprintf("IRQS: %u possible IRQs.\n", irqCount);
    irqHandlers = kheap_alloc(sizeof(irq_handler_t) * irqCount);
    memset(irqHandlers, 0, sizeof(irq_handler_t) * irqCount);

    // Open gates in IDT.
    for (uint8_t irq = 0; irq < irqCount; irq++)
        idt_open_interrupt_gate(idt, irq + IRQ_OFFSET, (uintptr_t)_irq_common);
    kprintf("IRQS: Initialized!\n");
}
