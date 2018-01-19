#include "main.h"
#include "io.h"

/**
 * Enable non-maskable interrupts
 */
void NMI_enable() {
	outb(0x70, inb(0x70)&0x7F);
}

/**
 * Disable non-maskable interrupts
 */
void NMI_disable() {
	outb(0x70, inb(0x70)|0x80);
}