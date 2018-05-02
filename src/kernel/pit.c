#include <main.h>
#include <io.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/pit.h>

#include <kernel/interrupts/irqs.h>
#include <kernel/tasking.h>

// https://wiki.osdev.org/Programmable_Interval_Timer

// Variable to hold the amount of ticks since the OS started.
static uint64_t ticks = 0;

// Return the number of ticks elapsed.
uint64_t pit_ticks(void) {
	return ticks;
}

// Starts a counter on the PIT.
void pit_startcounter(uint32_t freq, uint8_t counter, uint8_t mode) {
	// Determine divisor.
	uint16_t divisor = PIT_BASE_FREQ / freq;

	// Send command to PIT.
	uint8_t command = 0;
	command = (command & ~PIT_CMD_MASK_BINCOUNT) | PIT_CMD_BINCOUNT_BINARY;
	command = (command & ~PIT_CMD_MASK_MODE) | mode;
	command = (command & ~PIT_CMD_MASK_ACCESS) | PIT_CMD_ACCESS_DATA;
	command = (command & ~PIT_CMD_MASK_COUNTER) | counter;
	outb(PIT_PORT_COMMAND, command);

	// Send divisor.
	switch (counter) {
		case PIT_CMD_COUNTER0:
			outb(PIT_PORT_CHANNEL0, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL0, (divisor >> 8) & 0xFF);
			break;

		case PIT_CMD_COUNTER1:
			outb(PIT_PORT_CHANNEL1, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL1, (divisor >> 8) & 0xFF);
			break;

		case PIT_CMD_COUNTER2:
			outb(PIT_PORT_CHANNEL2, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL2, (divisor >> 8) & 0xFF);
			break;
	}
}

// Callback for PIT channel 0 on IRQ0.
static bool pit_callback(irq_regs_t *regs, uint8_t irqNum) {	
	// Increment the number of ticks.
	ticks++;

	// Change tasks every 5ms.
	if (ticks % 5 == 0)
		tasking_tick(regs);
	return true;
}

// Initialize the PIT.
void pit_init(void) {
	kprintf("PIT: Initializing...\n");

	// Start main timer at 1 tick = 2 ms.
	pit_startcounter(1000, PIT_CMD_COUNTER0, PIT_CMD_MODE_SQUAREWAVEGEN);

	// Register the handler with IRQ 0.
    irqs_install_handler(IRQ_PIT, pit_callback);

    // Wait for a tick to happen
    kprintf("PIT: Waiting for response...\n");
	while (ticks == 0);

	kprintf("PIT: Initialized!\n");
}
