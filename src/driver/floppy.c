#include <main.h>
#include <tools.h>
#include <io.h>
#include <logging.h>
#include <driver/floppy.h>
#include <kernel/interrupts.h>

// https://forum.osdev.org/viewtopic.php?t=13538

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

static void floppy_lba_to_chs(uint32_t lba, uint16_t* cyl, uint16_t* head, uint16_t* sector)
{
	*cyl = lba / (2 * FLPY_SECTORS_PER_TRACK);
	*head = ((lba % (2 * FLPY_SECTORS_PER_TRACK)) / FLPY_SECTORS_PER_TRACK);
	*sector = ((lba % (2 * FLPY_SECTORS_PER_TRACK)) % FLPY_SECTORS_PER_TRACK + 1);
}

void floppy_write_command(uint8_t cmd)
{
	for (uint16_t i = 0; i < 300; i++)
	{
		// Wait until register is ready.
		if(inb(FLOPPY_MAIN_STATUS_REGISTER) & FLOPPY_RQM) {
			outb(FLOPPY_DATA_FIFO, cmd);
			return;
		}
		sleep(10);
	}
	log("FLOPPY DRIVE DATA TIMEOUT\n");
}

uint8_t floppy_read_data()
{
	for (uint16_t i = 0; i < 300; i++)
	{
		// Wait until register is ready.
		if(inb(FLOPPY_MAIN_STATUS_REGISTER) & FLOPPY_RQM) {
			return inb(FLOPPY_DATA_FIFO);
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

static void floppy_sense_interrupt(uint8_t* st0, uint8_t* cyl)
{
	floppy_write_command(FLOPPY_SENSE_INTERRUPT);
	*st0 = floppy_read_data();
	*cyl = floppy_read_data();
}

static void floppy_set_drive_data(uint8_t step_rate, uint8_t load_time, uint8_t unload_time, bool dma)
{
	// Send specify command.
	floppy_write_command(FLOPPY_SPECIFY);

	// Send data.
	uint8_t data = ((step_rate & 0xF) << 4) | (unload_time & 0xF);
	floppy_write_command(data);
	data = (load_time << 1) | dma ? 0 : 1;
	floppy_write_command(data);
}

static void floppy_recalibrate(uint8_t drive)
{
	// Recalibrate the drive.
	floppy_write_command(FLOPPY_RECALIBRATE);
	floppy_write_command(drive);
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

void floppy_reset() {
	// Disable and re-enable floppy controller.
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, 0x00);
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, FLOPPY_IRQ | FLOPPY_RESET);
	floppy_wait_for_irq(300);

	// Set and lock base config
	log("Configuring floppy drive controller...\n");
	floppy_configure();
	log("Locking floppy drive controller configuration...\n");
	outb(FLOPPY_DATA_FIFO, FLOPPY_LOCK);

	// Clear any interrupts on drives.
	uint8_t* st0, cyl;
	for(int i = 0; i < 4; i++)
		floppy_sense_interrupt(&st0, &cyl);

	// Set transfer speed to 500 kb/s.
	outb(FLOPPY_CONFIGURATION_CONTROL_REGISTER, 0);

	// Set drive info (step time = 12ms, load time = 15ms, unload time = 0ms).
	floppy_set_drive_data(12, 15, 0, false);

	// Calibrate drive.
	floppy_recalibrate(0);
}

void floppy_seek(uint8_t drive, uint8_t cylinder) {
	floppy_write_command(FLOPPY_SEEK);
	floppy_write_command((0 << 2) | drive);
	floppy_write_command(cylinder);
	floppy_wait_for_irq(300);
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

	log("Turning on drive 0 motor...\n");
	floppy_set_motor(0, 1);

	log("Seeking drive 0 to cylinder 0...\n");
	floppy_seek(0, 1);

	sleep(1000);

	log("Seeking drive 0 to cylinder 60...\n");
	floppy_seek(0, 80);

	sleep(5000);

	log("Turning off drive 0 motor...\n");
	floppy_set_motor(0, 0);
}
