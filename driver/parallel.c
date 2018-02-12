#include <main.h>
#include <tools.h>
#include <io.h>

#define LPT1 0x378
#define LPT2 0x278
#define LPT3 0x3BC

void parallel_sendbyte(uint16_t port, unsigned char data) {
	unsigned char control;

	// Wait for device to be receptive
	while (!inb(port+1) & 0x80) {
		sleep(10);
	}

	// Write byte to data lines
	outb(port, data);

	// Pulse strobe line to tell end device to read data
	control = inb(port+2);
	outb(port+2, control | 1);
	sleep(10);
	outb(port+2, control);

	// Wait for end device to finish processing
	while (!inb(port+1) & 0x80) {
		sleep(10);
	}
}