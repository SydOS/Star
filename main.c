#include "main.h"
#include "driver/vga.h"
#include "driver/floppy.h"
#include "driver/nmi.h"

extern void enable_A20();

void kernel_main(void) {
	vga_initialize();
	vga_writes("SydOS Pre-Alpha\n");
	vga_writes("Starting up...\n");
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
		enable_A20();
    } else if (AX == 1) {
    	vga_writes("A20 line already enabled.\n");
    } else {
    	vga_writes("A20 line detection returned invalid result!\n");
    }

	vga_writes("HALTING CPU...\n");
}