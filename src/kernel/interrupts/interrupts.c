#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/interrupts/interrupts.h>

#include <kernel/interrupts/exceptions.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/irqs.h>
#include <driver/vga.h>

/**
 * Enable non-maskable interrupts
 */
void interrupts_nmi_enable(void) {
	outb(0x70, inb(0x70)&0x7F);
	kprintf("INTERRUPTS: NMI enabled!\n");
}

/**
 * Disable non-maskable interrupts
 */
void interrupts_nmi_disable(void) {
	outb(0x70, inb(0x70)|0x80);
	kprintf("INTERRUPTS: NMI disabled!\n");
}

// Initializes interrupts.
void interrupts_init(void) {
    kprintf("INTERRUPTS: Initializing...\n");

    // Initialize IDT.
    idt_init();

    // Initialize exceptions and IRQs.
    exceptions_init();
    irqs_init();
    interrupts_nmi_enable();

    // Enable interrupts.
	interrupts_enable();
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    kprintf("INTERRUPTS ARE ENABLED\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    kprintf("INTERRUPTS: Initialized!\n");
}
