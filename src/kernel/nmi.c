#include <main.h>
#include <io.h>
#include <kprint.h>

/**
 * Enable non-maskable interrupts
 */
void NMI_enable() {
	outb(0x70, inb(0x70)&0x7F);
	kprintf("NMI enabled!\n");
}

/**
 * Disable non-maskable interrupts
 */
void NMI_disable() {
	outb(0x70, inb(0x70)|0x80);
	kprintf("NMI disabled!\n");
}