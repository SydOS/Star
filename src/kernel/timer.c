#include <main.h>
#include <io.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/timer.h>

#include <driver/pit.h>
#include <kernel/interrupts/ioapic.h>
#include <kernel/interrupts/irqs.h>

// Variable to hold the amount of ticks since the OS started.
static uint64_t ticks = 0;

// Return the number of ticks elapsed.
uint64_t timer_ticks(void) {
	return ticks;
}

// Callback for timer on IRQ0.
static bool timer_callback(irq_regs_t *regs, uint8_t irqNum) {	
	// Increment the number of ticks.
	ticks++;

	// Change tasks every 5ms.
	if (ticks % 5 == 0)
		tasking_tick(regs);
	return true;
}

void timer_init(void) {
    kprintf("TIMER: Initializing...\n");

    // Initialize PIT.
    pit_init();

    // Register the handler with IRQ 0.
    irqs_install_handler(IRQ_TIMER, timer_callback);

    // Wait for a tick to happen.
    kprintf("TIMER: Waiting for response from PIT.\nTIMER: If the system hangs here, IRQs or the PIT are not working.\n");
	sleep(1);
    kprintf("TIMER: PIT test passed!\n");

    // Are APICs supported?
    if (ioapic_supported()) {
        // Initialize LAPIC timer.
        lapic_timer_init();

        // Test LAPIC timer.
        kprintf("TIMER: Waiting for response from LAPIC.\nTIMER: If the system hangs here, IRQs or the LAPIC are not working.\n");
	    sleep(10);
        kprintf("TIMER: LAPIC test passed!\n");
    }

    kprintf("TIMER: Initialized!\n");
}
