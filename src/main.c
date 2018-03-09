#include <main.h>
#include <tools.h>
#include <io.h>
#include <string.h>
#include <kprint.h>
#include <multiboot.h>
#include <arch/i386/kernel/gdt.h>
#include <arch/i386/kernel/idt.h>
#include <arch/i386/kernel/interrupts.h>
#include "kernel/nmi.h"
#include "kernel/pit.h"
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/kheap.h>
#include <kernel/tasking.h>
#include <arch/i386/kernel/cpuid.h>
#include "driver/vga.h"
#include "driver/floppy.h"
#include <driver/serial.h>
#include "driver/speaker.h"
#include "driver/ps2/ps2.h"
#include "driver/rtc.h"

// Displays a kernel panic message and halts the system.
void panic(const char *format, ...) {
	// Disable interrupts.
	asm volatile("cli");

    // Get args.
    va_list args;
	va_start(args, format);

	// Show panic.
	kprintf("\nPANIC:\n");
	kprintf_va(format, args);
	kprintf("\n\nHalted.");

	// Halt forever.
	while (true);
}

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main(multiboot_info_t* mboot_info) {
	// Ensure interrupts are disabled.
	asm volatile("cli");
	vga_disable_cursor();
	
	// Initialize serial for logging.
	serial_init();

	vga_initialize();
	vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
	vga_writes("   _____           _  ____   _____ \n");
	vga_writes("  / ____|         | |/ __ \\ / ____|\n");
	vga_writes(" | (___  _   _  __| | |  | | (___  \n");
	vga_writes("  \\___ \\| | | |/ _` | |  | |\\___ \\ \n");
	vga_writes("  ____) | |_| | (_| | |__| |____) |\n");
	vga_writes(" |_____/ \\__, |\\__,_|\\____/|_____/ \n");
	vga_writes("          __/ |                    \n");
	vga_writes("         |___/                     \n");
	vga_writes("Copyright (c) Sydney Erickson 2017 - 2018\n");

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	kprintf("Initializing GDT...\n");
	gdt_init();

	// -------------------------------------------------------------------------
	// MEMORY RELATED STUFF
	vga_setcolor(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
	
	// Initialize physical memory manager.
	kprintf("Initializing Physical memory manager...\n");
	pmm_init(mboot_info);

	// Initialize paging.
	kprintf("Initializing paging...\n");
    paging_init();

	// Initialize kernel heap.
	kprintf("Initializing kernel heap...\n");
	kheap_init();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	kprintf("Initializing ACPI...\n");
	acpi_init();

	kprintf("Initializing IDT...\n");
	idt_init();

	kprintf("Initializing interrupts...\n");
	interrupts_init();

	kprintf("Enabling NMI...\n");
	NMI_enable();
    
    // Enable interrupts.
	asm volatile("sti");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    kprintf("INTERRUPTS ARE ENABLED\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	kprintf("Setting up PIT...\n");
    pit_init();

	kprintf("Sleeping for 2 seconds...\n");
	sleep(2000);

	// Start up tasking and create kernel task.
	kprintf("Starting tasking...\n");
	tasking_init();

	// We should never get here.
	panic("Tasking failed to start!\n");
}

void hmmm_thread() {
	while (1) { 
		kprintf("hmm(): %u seconds\n", pit_ticks() / 1000);
		sleep(2000);
	 }
}

void kernel_late() {
	kprintf("Adding second task...\n");
	__addProcess(tasking_create_process("hmmm", (uint32_t)hmmm_thread));
	kprintf("Starting tasking...\n");

	// Initialize PS/2.
	vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);	
	kprintf("Initializing PS/2...\n");
	ps2_init();
	
	// Initialize floppy.
	vga_setcolor(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
	floppy_detect();
	kprintf("Initialize floppy drives...\n");
	floppy_init();

    vga_enable_cursor();

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

	// Print CPUID info.
	cpuid_print_capabilities();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);


	kprintf("Current uptime: %i milliseconds.\n", pit_ticks());
	
	vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
	kprintf("Kernel is located at 0x%X!\n", memInfo.kernelStart);
	kprintf("Detected usable RAM: %uMB\n", memInfo.memoryKb / 1024);
	if (memInfo.paeEnabled)
		kprintf("PAE enabled!\n");
	if (memInfo.nxEnabled)
		kprintf("NX enabled!\n");

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	rtc_init();
	kprintf("24 hour time: %d, binary input: %d\n", rtc_settings->twentyfour_hour_time, rtc_settings->binary_input);
	struct RTCTime* time = rtc_get_time();
	kprintf("%d:%d:%d %d/%d/%d\n", time->hours, time->minutes, time->seconds, time->month, time->day, time->year);
	kheap_free(time);

    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	kprintf("root@sydos ~: ");
	serial_writes("root@sydos ~: ");
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    // Ring serial and VGA terminals.
	serial_write('\a');
	vga_putchar('\a');
  
	// If serial isn't present, just loop.
	if (!serial_present()) {
		kprintf("No serial port present for logging, waiting here.");
		while (true);
	}

	for(;;) {
		char c = serial_read();

		if (c == '\r' || c == '\n') {
			vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
			kprintf("\nroot@sydos ~: ");
			serial_writes("\nroot@sydos ~: ");
			vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		} else {
			vga_putchar(c);
			serial_write(c);
			vga_trigger_cursor_update();
		}
	}
}
