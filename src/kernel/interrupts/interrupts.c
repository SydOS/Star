#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/interrupts/interrupts.h>

#include <kernel/interrupts/exceptions.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/irqs.h>
#include <driver/vga.h>

#define NMI_PORT 0x70

void interrupts_enable(void) {
    asm volatile ("sti");
    kprintf("\e[97;44m   INTERRUPTS ARE ENABLED   \e[0m\n");
}

void interrupts_disable(void) {
    asm volatile ("cli");
    kprintf("\e[97;44m   INTERRUPTS ARE DISABLED   \e[0m\n");
}

/**
 * Enable non-maskable interrupts
 */
void interrupts_nmi_enable(void) {
	outb(NMI_PORT, inb(NMI_PORT) & 0x7F);
	kprintf("INTERRUPTS: NMI enabled!\n");
}

/**
 * Disable non-maskable interrupts
 */
void interrupts_nmi_disable(void) {
	outb(NMI_PORT, inb(NMI_PORT) | 0x80);
	kprintf("INTERRUPTS: NMI disabled!\n");
}

void interrupts_init_ap(void) {
    kprintf("INTERRUPTS: Initializing for AP...\n");

    // Load existing IDT.
    idt_load(idt_get_bsp());

    // Enable interrupts.
	interrupts_enable();
    kprintf("\e[33mINTERRUPTS: Initialized!\e[0m\n");
}

// Initializes interrupts.
void interrupts_init_bsp(void) {
    kprintf("\e[33mINTERRUPTS: Initializing...\n");

    // Initialize IDT.
    idt_init_bsp();
    idt_entry_t *bspIdt = idt_get_bsp();

    // Initialize exceptions and IRQs.
    exceptions_init(bspIdt);
    irqs_init(bspIdt);
    interrupts_nmi_enable();

    // Enable interrupts.
	interrupts_enable();
    kprintf("\e[33mINTERRUPTS: Initialized!\e[0m\n");
}
