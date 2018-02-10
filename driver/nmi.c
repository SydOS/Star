#include <main.h>
#include <io.h>
#include <logging.h>

/**
 * Enable non-maskable interrupts
 */
void NMI_enable() {
	outb(0x70, inb(0x70)&0x7F);
	log("NMI enabled!\n");
}

/**
 * Disable non-maskable interrupts
 */
void NMI_disable() {
	outb(0x70, inb(0x70)|0x80);
	log("NMI disabled!\n");
}