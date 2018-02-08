#include "main.h"
#include "tools.h"
#include "driver/gdt.h"
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/nmi.h"
#include "driver/pic.h"
#include "driver/idt.h"
#include "driver/pit.h"
#include "driver/memory.h"

#include "logging.h"

/**
 * Kernel's ending address in RAM
 */
extern uint32_t kernel_end;
/**
 * Kernel's starting address in RAM
 */
extern uint32_t kernel_base;

/**
 * Function in enable_a20.asm to enable the A20 line.
 * This should be moved to a header file :(
 */
extern void _enable_A20();

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main(void) {
	serial_initialize();
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
	
	// -------------------------------------------------------------------------
	// MEMORY RELATED STUFF

	vga_setcolor(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);

	vga_writes("Checking A20 line...\n");
	int AX;
	asm( "movl $0, %0"
   		: "=a" (AX)
    );
    if (AX == 0) {
    	vga_writes("Enabling A20 line...\n");
		_enable_A20();
    } else if (AX == 1) {
    	vga_writes("A20 line already enabled.\n");
    } else {
    	vga_writes("A20 line detection returned invalid result!\n");
    }

	char kernbase[32], kernend[32];
	itoa((uint32_t)&kernel_base, kernbase, 16);
	itoa((uint32_t)&kernel_end, kernend, 16);
	log("Kernel start: 0x");
	log(kernbase);
	log(" | Kernel end: 0x");
	log(kernend);
	log("\n");

	memory_init((uint32_t)&kernel_end);
	memory_print_out();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	// -------------------------------------------------------------------------

	log("Initializing GDT...\n");
	gdt_init();

	log("Initializing IDT...\n");
	idt_init();

	log("Enabling NMI...\n");
	NMI_enable();

	// TODO: Setup exceptions in our IDT table

    log("Setting up PIC...\n");
    PIC_remap(0x20, 0x28);

    //log("Setting up PIT...\n");
    //pit_init();
    
    // Enable interrupts
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    asm volatile("sti");
    log("INTERRUPTS ARE ENABLED\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
	log("HALTING CPU...\n");
}