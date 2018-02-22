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
#include "kernel/memory.h"
#include "kernel/paging.h"
#include <arch/i386/kernel/cpuid.h>
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/serial.h"
#include "driver/speaker.h"
#include "driver/ps2/ps2.h"

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main(uint32_t mboot_magic, multiboot_info_t* mboot_info)
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
	
	// Ensure multiboot magic value is good.
	if (mboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		kprintf("MULTIBOOT BOOTLOADER MAGIC NUMBER 0x%X IS INVALID!\n", mboot_magic);
		// Kernel should die at this point.....
		return;
	}




	kprintf("Initializing GDT...\n");
	gdt_init();

	kprintf("Initializing IDT...\n");
	idt_init();

	kprintf("Initializing interrupts...\n");
	interrupts_init();

	kprintf("Enabling NMI...\n");
	NMI_enable();

	//vga_setcolor(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
	//log("Kernel has entered protected mode.\n");
	//vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    // Enable interrupts.
	asm volatile("sti");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    kprintf("INTERRUPTS ARE ENABLED\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);


	
	// -------------------------------------------------------------------------
	// MEMORY RELATED STUFF
	vga_setcolor(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
	

	memory_init(mboot_info);
	//memory_print_out();



		kprintf("Setting up PIT...\n");
    pit_init();

		kprintf("Sleeping for 5 seconds...\n");
	sleep(5000);

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

	// Print CPUID info.
	cpuid_print_capabilities();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	// -------------------------------------------------------------------------

    kprintf("Initializing paging...\n");
    paging_initialize();

	kprintf("Initializing PS/2...\n");
	ps2_init();

	// Floppy test.
	floppy_detect();
	kprintf("Initialize floppy drives...\n");
	floppy_init();

    vga_enable_cursor();

	kprintf("Current uptime: %i milliseconds.\n", pit_ticks());

    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	vga_writes("root@sydos ~: ");
	serial_writes("root@sydos ~: ");
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    // Ring serial and VGA terminals.
	serial_write('\a');
	vga_putchar('\a');

	/*char* bee_movie = "According to all known laws\r\nof aviation,\r\nthere is no way a bee\r\nshould be able to fly.\r\nIts wings are too small to get\r\nits fat little body off the ground.\r\nThe bee, of course, flies anyway\r\nbecause bees don't care\r\nwhat humans think is impossible.\r\nYellow, black. Yellow, black.\r\nYellow, black. Yellow, black.\r\nOoh, black and yellow!\r\nLet's shake it up a little.\r\nBarry! Breakfast is ready!";
	for (size_t i = 0; i < strlen(bee_movie); i++)
	{
		parallel_sendbyte(0x378, bee_movie[i]);
	}

	// Send eject page PCL command.
	parallel_sendbyte(0x378, 0x1B);
	parallel_sendbyte(0x378, 0x26);
	parallel_sendbyte(0x378, 0x6C);
	parallel_sendbyte(0x378, 0x30);
	parallel_sendbyte(0x378, 0x48);*/

	for(;;) {
		char* input[80];
		int i = 0;
		char c = serial_read();

		if (c == '\r' || c == '\n') {
			vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
			vga_writes("\nroot@sydos ~: ");
			serial_writes("\nroot@sydos ~: ");
			vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		} else {
			input[i] = &c;
			i++;
			vga_putchar(c);
			serial_write(c);
		}
	}
}
