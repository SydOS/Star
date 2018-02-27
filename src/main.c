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
#include "kernel/memory.h"
#include "kernel/paging.h"
#include <arch/i386/kernel/cpuid.h>
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/serial.h"
#include "driver/speaker.h"
#include "driver/ps2/ps2.h"

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
void kernel_main(multiboot_info_t* mboot_info)
{
	// Ensure interrupts are disabled.
	asm volatile("cli");
	vga_disable_cursor();
	
	serial_initialize();
	const char* data = "If you're reading this, serial works.\n";
	for (size_t i = 0; i < strlen(data); i++) {
		serial_write(data[i]);
	}

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
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

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

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

	// Print CPUID info.
	cpuid_print_capabilities();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	// -------------------------------------------------------------------------



	kprintf("Initializing PS/2...\n");
	ps2_init();

	// Floppy test.
	floppy_detect();
	kprintf("Initialize floppy drives...\n");
	floppy_init();

    vga_enable_cursor();

	kprintf("Current uptime: %i milliseconds.\n", pit_ticks());
	
	vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
	kprintf("Kernel is located at 0x%X!\n", memInfo.kernelStart);
	kprintf("Detected usable RAM: %uMB\n", memInfo.memoryKb / 1024);

    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	kprintf("root@sydos ~: ");
	serial_writes("root@sydos ~: ");
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	

    // Ring serial and VGA terminals.
	serial_write('\a');
	vga_putchar('\a');

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
