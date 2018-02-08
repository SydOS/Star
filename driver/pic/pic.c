#include "main.h"
#include "driver/pic.h"
#include "io.h"
#include "logging.h"

/**
 * Send end of interrupt command to PIC
 * @param irq IRQ interrupt ID
 */
void PIC_sendEOI(unsigned char irq) {
	if(irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}

	outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Remaps the PIC to certain ports
 * @param offset1 Vector offset for master PIC (offset1...offset1+7)
 * @param offset2 Vector offset for slave PIC (offset2...offset2+7)
 */
void PIC_remap(int offset1, int offset2) {
	unsigned char a1, a2;

	a1 = inb(PIC1_DATA);                     // Save masks
	a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4); // Start init sequence in cascade mode
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
	io_wait();

	outb(PIC1_DATA, offset1);                // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                // ICW2: Slave PIC vector offset
	io_wait();

	outb(PIC1_DATA, 4);                      // ICW3: tell master PIC that slave exists at IRQ2
	io_wait();
	outb(PIC2_DATA, 2);                      // ICW3: tell slave PIC it's cascade identity
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, a1);
	outb(PIC2_DATA, a2);

	log("PIC remapped!\n");
}