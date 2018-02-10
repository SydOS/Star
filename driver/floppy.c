#include <main.h>
#include <io.h>
#include <logging.h>
#include <driver/floppy.h>

/**
 * Temporary function to detect floppy disks in drives
 */
void floppy_detect() {
	unsigned char a, b, c;
	outb(0x70, 0x10);
	c = inb(0x71);

	a = c >> 4; // Get high nibble
	b = c & 0xF; // Get low nibble by ANDing out low nibble

	char *drive_type[6] = { "no floppy drive", "360kb 5.25in floppy drive", "1.2mb 5.25in floppy drive", "720kb 3.5in", "1.44mb 3.5in", "2.88mb 3.5in"};
	log("Floppy drive A: ");
	log(drive_type[a]);
	log("\nFloppy drive B: ");
	log(drive_type[b]);
	log("\n");
}

unsigned char floppy_getversion() {
	// Get version of floppy controller.
	outb(FLOPPY_DATA_FIFO, FLOPPY_VERSION);
	return inb(FLOPPY_DATA_FIFO);
}

void floppy_reset() {
	// Reset floppy controller.
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, 0x00);
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, 0x0C);
}

void floppy_configure() {
	// Send configure command.
	outb(FLOPPY_DATA_FIFO, FLOPPY_CONFIGURE);
}

void floppy_init() {
	// Get controller version.
	char* temp1;
	uint32_t version = floppy_getversion();

	// If version is 0xFF, that means there isn't a floppy controller.
	if (version == 0xFF)
	{
		log("No floppy controller present, aborting!\n");
		return;
	}

	// Print version for now.
	utoa(version, temp1, 16);
	log("Floppy controller version is 0x");
	log(temp1);
	log("!\n");

	floppy_reset();
}
