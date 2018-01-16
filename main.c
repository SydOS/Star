#include "main.h"
#include "driver/vga.h"
#include "driver/floppy.h"
#include "drivers/nmi.h"

void kernel_main(void) {
	vga_initialize();
	vga_writes("SydOS Pre-Alpha\n");
	vga_writes("Starting up...\n");
	vga_writes("Detecting floppy disks...\n")
	floppy_detect();
	vga_writes("Disabling NMI...\n");
	NMI_disable();
}