#include "main.h"
#include "driver/vga.h"
#include "driver/floppy.h"

void kernel_main(void) {
	vga_initialize();
	vga_writes("SydOS Pre-Alpha\n");
	vga_writes("Starting up...\n");
	floppy_detect();
}