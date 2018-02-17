#include <main.h>
#include <tools.h>
#include <io.h>
#include <logging.h>
#include <driver/floppy.h>
#include <kernel/interrupts.h>

static bool irq_triggered = false;

/**
 * Temporary function to detect floppy disks in drives
 */
void floppy_detect() {
	unsigned char a, b, c;
	outb(0x70, 0x10);
	c = inb(0x71);

	a = c >> 4; // Get high nibble
	b = c & 0xF; // Get low nibble by ANDing out low nibble

	char *drive_type[6] = { "no floppy drive", "360kb 5.25in floppy drive", 
	"1.2mb 5.25in floppy drive", "720kb 3.5in", "1.44mb 3.5in", "2.88mb 3.5in"};
	log("Floppy drive A: ");
	log(drive_type[a]);
	log("\nFloppy drive B: ");
	log(drive_type[b]);
	log("\n");
}

void floppy_write_command(uint8_t cmd)
{
	uint16_t timeout = 300;
	while(--timeout) {
		if(inb(FLOPPY_MAIN_STATUS_REGISTER) & FLOPPY_RQM) {
			outb(FLOPPY_DATA_FIFO, cmd);
			return;
		}
		sleep(10);
	} 
	log("FLOPPY DRIVE DATA TIMEOUT\n");
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

void floppy_irq(registers_t* regs) {
	irq_triggered = true;
	log("IRQ6 raised!\n");
}

unsigned char floppy_getversion() {
	// Get version of floppy controller.
	outb(FLOPPY_DATA_FIFO, FLOPPY_VERSION);
	return inb(FLOPPY_DATA_FIFO);
}

void floppy_reset() {
	// Reset floppy controller.
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, 0x00);
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, FLOPPY_IRQ | FLOPPY_RESET);
	floppy_wait_for_irq(300);
}

void floppy_configure() {
	// Send configure command.
	floppy_write_command(FLOPPY_CONFIGURE);
	floppy_write_command(0x0);
	char data = (1 << 6) | (1 << 5) | (0 << 4) | (15 - 1);
	floppy_write_command(data);
	floppy_write_command(0x0);
}

void floppy_recalibrate(uint8_t drive) {
	floppy_write_command(FLOPPY_RECALIBRATE);
	floppy_write_command(drive);
	floppy_wait_for_irq(300);
}

void floppy_seek(uint8_t drive, uint8_t cylinder) {
	floppy_write_command(FLOPPY_SEEK);
	floppy_write_command((0 << 2) | drive);
	floppy_write_command(cylinder);
	floppy_wait_for_irq(300);
}

void floppy_drive_set(uint8_t step, uint8_t loadt, uint8_t unloadt, uint8_t dma)
{
	floppy_write_command(FLOPPY_SPECIFY);
	uint8_t data = 0;
	data = ((step&0xf)<<4) | (unloadt & 0xf);
	floppy_write_command(data);
	data = ((loadt << 1) | (dma?0:1));
	floppy_write_command(data);
}

void floppy_set_motor(uint8_t drive, uint8_t status) {
	uint8_t motor = 0;
	switch(drive)
	{
		case 0:
			motor = FLOPPY_MOTA;
			break;
		case 1:
			motor = FLOPPY_MOTB;
			break;
		case 2:
			motor = FLOPPY_MOTC;
			break;
		case 3:
			motor = FLOPPY_MOTD;
			break;
	}

	if(status) {
		outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, motor | FLOPPY_IRQ | FLOPPY_RESET);
	} else {
		outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, FLOPPY_IRQ | FLOPPY_RESET);
	}
	sleep(500);
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
	interrupts_irq_install_handler(6, floppy_irq);

	// Print version for now.
	utoa(version, temp1, 16);
	log("Floppy controller version is 0x");
	log(temp1);
	log("!\n");

		// Reset floppy drive
	log("Resetting floppy drive controller...\n");
	floppy_reset();

	// Set and lock base config
	log("Configuring floppy drive controller...\n");
	floppy_configure();
	log("Locking floppy drive controller configuration...\n");
	outb(FLOPPY_DATA_FIFO, FLOPPY_LOCK);

	// Reset floppy drive
	log("Resetting floppy drive controller...\n");
	floppy_reset();

	log("Recalibrating floppy drives...\n");
	floppy_recalibrate(0);

	log("Turning on drive 0 motor...\n");
	floppy_set_motor(0, 1);

	log("Seeking drive 0 to cylinder 0...\n");
	floppy_seek(0, 1);

	log("Turning off drive 0 motor...\n");
	floppy_set_motor(0, 0);
}
