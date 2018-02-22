#include <main.h>
#include <io.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/pit.h>
#include <kernel/tasking.h>
#include <arch/i386/kernel/interrupts.h>

// https://wiki.osdev.org/Programmable_Interval_Timer

// Variable to hold the amount of ticks since the OS started.
uint64_t ticks;
uint8_t task = 0;
uint8_t task_was_on = 0;

// Set currently running task
void pit_set_task(uint8_t i)
{
	if(!task_was_on) return;
	task = i;
}

// Enable tasking
void pit_enable_tasking() {
	task_was_on = 1;
	task = 1;
}

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
static void pit_callback(registers_t* regs) {	
	// Increment the number of ticks.
	ticks++;
	if (task_was_on) { tasking_tick(); }
}

// Initialize the PIT.
void pit_init() {
	// Start main timer at 1 tick = 1 ms.
	pit_startcounter(1000, PIT_CMD_COUNTER0, PIT_CMD_MODE_RATEGEN);

	// Register the handler with IRQ 0.
    interrupts_irq_install_handler(0, pit_callback);
	kprintf("PIT initialized!\n");
}
