#include <main.h>
#include <tools.h>
#include <io.h>
#include <logging.h>
#include <driver/floppy.h>
#include <kernel/interrupts.h>

bool irq_triggered = false;

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

void floppy_wait_for_irq(uint16_t timeout) {
	uint8_t ret = 0;
	while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(10);
	}
	if(irq_triggered) { ret = 1; } else { log("FLOPPY DRIVE IRQ TIMEOUT\n"); }
	irq_triggered = false;
	return ret;
}

void floppy_irq() {
	irq_triggered = true;
	//send_eoi(6);
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
	outb(FLOPPY_DATA_FIFO, 0x0);
	char data = (1 << 6) | (1 << 5) | (0 << 4) | (15 - 1);
	outb(FLOPPY_DATA_FIFO, data);
	outb(FLOPPY_DATA_FIFO, 0x0);
}

void floppy_recalibrate(uint8_t drive) {
	outb(FLOPPY_DATA_FIFO, FLOPPY_RECALIBRATE);
	outb(FLOPPY_DATA_FIFO, drive);
	floppy_wait_for_irq(300);
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
	} else if (version != 0x90) {
		log("Unofficially supported floppy controller version. Proceed with caution!\n");
	}

	log("Installing floppy drive IRQ...\n");
	interrupts_irq_install_handler(38, &floppy_irq);

	// Print version for now.
	utoa(version, temp1, 16);
	log("Floppy controller version is 0x");
	log(temp1);
	log("!\n");

	// Set and lock base config
	log("Configuring floppy drive controller...\n");
	floppy_configure();
	log("Locking floppy drive controller configuration...\n");
	outb(FLOPPY_DATA_FIFO, FLOPPY_LOCK);

	// Reset floppy drive
	log("Resetting floppy drive controller...\n");
	floppy_reset();

	log("Recalibrating floppy drives...\n");
	floppy_recalibrate(0x0);
}
