#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/parallel.h>

// https://wiki.osdev.org/Parallel_port
// http://retired.beyondlogic.org/spp/parallel.htm
// http://retired.beyondlogic.org/epp/epp.htm

#define LPT1 0x378
#define LPT2 0x278
#define LPT3 0x3BC

void parallel_reset(uint16_t port)
{
	outb(PARALLEL_CONTROL_PORT(port), 0x00);
	sleep(10);
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);
}

void parallel_sendbyte(uint16_t port, unsigned char data)
{
	// Wait for device to be receptive.
	while (!inb(PARALLEL_STATUS_PORT(port)) && PARALLEL_STATUS_BUSY)
		sleep(10);

	// Write byte to data port.
	outb(port, data);

	// Pulse strobe line to tell end device to read data.
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_STROBE | PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);
	sleep(10);
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);

	// Wait for end device to finish processing.
	while (!inb(PARALLEL_STATUS_PORT(port)) && PARALLEL_STATUS_BUSY)
		sleep(10);
}
