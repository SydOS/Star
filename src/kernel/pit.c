#include <main.h>
#include <io.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/pit.h>
#include <arch/i386/kernel/interrupts.h>
#include <kernel/tasking.h>

// https://wiki.osdev.org/Programmable_Interval_Timer

// Variable to hold the amount of ticks since the OS started.
static uint64_t ticks = 0;

// Return the number of ticks elapsed.
uint64_t pit_ticks() {
	return ticks;
}

// Starts a counter on the PIT.
void pit_startcounter(uint32_t freq, uint8_t counter, uint8_t mode) {
	// Determine divisor.
	uint16_t divisor = 1193180 / freq;

	// Send command to PIT.
	uint8_t command = 0;
	command = (command & ~PIT_CMD_MASK_MODE) | mode;
	command = (command & ~PIT_CMD_MASK_ACCESS) | PIT_CMD_ACCESS_DATA;
	command = (command & ~PIT_CMD_MASK_COUNTER) | counter;
	outb(PIT_PORT_COMMAND, command);

	// Send divisor.
	switch (counter)
	{
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
static void pit_callback(registers_t *regs)
{	
	// Increment the number of ticks.
	ticks++;

	// Change tasks.
	tasking_tick(regs);
}

// Initialize the PIT.
void pit_init() {
	// Start main timer at 1 tick = 1 ms.
	pit_startcounter(1000, PIT_CMD_COUNTER0, PIT_CMD_MODE_RATEGEN);

	// Register the handler with IRQ 0.
    interrupts_irq_install_handler(0, pit_callback);

    // Wait for a tick to happen
    kprintf("Waiting for PIT test...\n");
	while (ticks == 0) {

	}

	kprintf("PIT initialized!\n");
}
