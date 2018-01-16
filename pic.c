#include "main.h"
#include "driver/pic.h"
#include "io.h"

void PIC_sendEOI(unsigned char irq) {
	if(irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}

	outb(PIC1_COMMAND, PIC_EOI);
}