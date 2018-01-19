#include "main.h"
#include "driver/gdt.h"
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/nmi.h"
#include "driver/pic.h"

extern void _enable_A20();

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main(void) {
	vga_initialize();
	vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
	vga_writes("   _____           _  ____   _____ \n  / ____|         | |/ __ \\ / ____|\n | (___  _   _  __| | |  | | (___  \n  \\___ \\| | | |/ _` | |  | |\\___ \\ \n  ____) | |_| | (_| | |__| |____) |\n |_____/ \\__, |\\__,_|\\____/|_____/ \n          __/ |                    \n         |___/                     \n");
	vga_writes("Copyright (c) Sydney Erickson 2017 - 2018\n");
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	vga_writes("Initializing GDT...\n");
	gdt_init();

	vga_writes("Detecting floppy disks...\n");
	floppy_detect();

	vga_writes("Enabling NMI...\n");
	NMI_enable();

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

    vga_writes("Setting up PIC...\n");
    PIC_remap(0x20, 0x28);

    vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
	vga_writes("HALTING CPU...\n");
}