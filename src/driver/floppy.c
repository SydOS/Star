#include <main.h>
#include <tools.h>
#include <io.h>
#include <logging.h>
#include <driver/floppy.h>
#include <kernel/interrupts.h>

// https://forum.osdev.org/viewtopic.php?t=13538
// www.osdever.net/documents/82077AA_FloppyControllerDatasheet.pdf

static bool irq_triggered = false;
static bool implied_seeks = false;

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
		sleep(20);
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
		sleep(20);
	}
	log("FLOPPY DRIVE DATA TIMEOUT\n");
}

bool floppy_wait_for_irq(uint16_t timeout) {
	uint8_t ret = false;
	while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(20);
	}
	if(irq_triggered) { ret = true; } else { log("FLOPPY DRIVE IRQ TIMEOUT\n"); }
	irq_triggered = false;
	return ret;
}

void floppy_irq(registers_t* regs) {
	irq_triggered = true;
	log("IRQ6 raised!\n");
}

static uint8_t floppy_getversion()
{
	// Get version of floppy controller.
	floppy_write_command(FLOPPY_VERSION);
	return floppy_read_data();
}

static void floppy_sense_interrupt(uint8_t* st0, uint8_t* cyl)
{
	floppy_write_command(FLOPPY_SENSE_INTERRUPT);
	*st0 = floppy_read_data();
	*cyl = floppy_read_data();
}

static void floppy_set_drive_data(uint8_t step_rate, uint16_t load_time, uint8_t unload_time, bool dma)
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

// Configure default values.
// EIS - No Implied Seeks
// EFIFO - FIFO Disabled
// POLL - Polling Enabled
// FIFOTHR - FIFO Threshold Set to 1 Byte
// PRETRK - Pre-Compensation Set to Track 0
void floppy_configure(bool eis, bool efifo, bool poll, uint8_t fifothr, uint8_t pretrk)
{
	// Send configure command.
	floppy_write_command(FLOPPY_CONFIGURE);
	floppy_write_command(0x0);
	char data = (!eis << 6) | (!efifo << 5) | (poll << 4) | fifothr;
	floppy_write_command(data);
	floppy_write_command(pretrk);
}

void floppy_reset() {
	// Disable and re-enable floppy controller.
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, 0x00);
	outb(FLOPPY_DIGITAL_OUTPUT_REGISTER, FLOPPY_IRQ | FLOPPY_RESET);
	floppy_wait_for_irq(300);

	// Clear any interrupts on drives.
	log("Clearing interrupts...\n");
	uint8_t* st0, cyl;
	for(int i = 0; i < 4; i++)
		floppy_sense_interrupt(&st0, &cyl);
}

bool floppy_seek(uint8_t drive, uint8_t cylinder)
{
	char* tmp;
	uint8_t st0, cyl = 0;

	// Attempt seek up to 10 times.
	for(uint8_t i = 0; i < 10; i++)
	{
		// Send seek command.
		log("Seeking to track ");
		log(utoa(cylinder, tmp, 10));
		log("...\n");
		floppy_write_command(FLOPPY_SEEK);
		floppy_write_command((0 << 2) | drive);
		floppy_write_command(cylinder);

		// Wait for response and check interrupt.
		floppy_wait_for_irq(300);
		floppy_sense_interrupt(&st0, &cyl);

		// Ensure command completed successfully.
		if (st0 & 0xC0)
		{
			log("Error executing floppy seek command!\n");
			continue;
		}
		
		// If we have reached the request cylinder, return.
		if (cyl == cylinder)
			return true;
	}

	// Seek failed if we get here.
	return false;
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

uint8_t floppy_read_sector(uint32_t sector_lba)
{
	// Convert LBA to CHS.
	uint16_t head = 0, track = 0, sector = 1;
	uint8_t st0, cyl = 0;
	floppy_lba_to_chs(sector_lba, &head, &track, &sector);

	// Turn on motor and seek to track.
	floppy_set_motor(0, 1);
	if (!floppy_seek(0, track))
		return 0;

	// Send read command to disk.
	floppy_write_command(FLOPPY_READ_DATA | 0x40 | 0x80);
	floppy_write_command(head << 2 | 0);
	floppy_write_command(track);
	floppy_write_command(head);
	floppy_write_command(sector);
	floppy_write_command(2);
	floppy_write_command(1);
	floppy_write_command(0x1B);
	floppy_write_command(0xFF);

	// Wait for IRQ.
	floppy_wait_for_irq(1000);

	// Read data.
	uint8_t data[512];
	char* tmp;
	for(uint8_t i = 0; i < 54; i++)
	{
		data[i] = floppy_read_data();
		log(utoa(data[i], tmp, 16));
		log(" ");
	}

	// Turn off motor.
	floppy_set_motor(0, 0);
}

void floppy_init()
{
	// Install IRQ.
	interrupts_irq_install_handler(6, floppy_irq);

	// Reset floppy drive.
	log("Resetting floppy drive controller...\n");
	floppy_reset();

	// Get controller version.	
	uint32_t version = floppy_getversion();

	// If version is 0xFF, that means there isn't a floppy controller.
	if (version == 0xFF)
	{
		log("No floppy controller present, aborting!\n");
		return;
	}

	// Print version for now.
	char* temp1;
	utoa(version, temp1, 16);
	log("Floppy controller version is 0x");
	log(temp1);
	log("!\n");

	// Set and lock base config.
	log("Configuring floppy drive controller...\n");
	implied_seeks = FLOPPY_VERSION_ENHANCED;
	floppy_configure(implied_seeks, true, false, 0, 0);
	log("Locking floppy drive controller configuration...\n");
	outb(FLOPPY_DATA_FIFO, FLOPPY_LOCK);

	// Reset floppy drive.
	log("Resetting floppy drive controller...\n");
	floppy_reset();

	// Set transfer speed to 500 kb/s.
	log("Setting transfer speed...\n");
	outb(FLOPPY_CONFIGURATION_CONTROL_REGISTER, 0);

	// Set drive info (step time = 4ms, load time = 16ms, unload time = 240ms).
	log("Setting floppy drive info...\n");
	floppy_set_drive_data(0xC, 0x2, 0xF, false);

	// Calibrate drive.
	log("Calibrating floppy drive...\n");
	floppy_recalibrate(0);

	log("Turning on drive 0 motor...\n");
	floppy_set_motor(0, 1);

	/*log("Seeking drive 0 to cylinder 0...\n");
	floppy_seek(0, 1);

	sleep(1000);

	log("Seeking drive 0 to cylinder 60...\n");
	floppy_seek(0, 80);
	log("Seeking drive 0 to cylinder 60...\n");
	floppy_seek(0, 1);
	log("Seeking drive 0 to cylinder 60...\n");
	floppy_seek(0, 80);
	log("Seeking drive 0 to cylinder 60...\n");
	floppy_seek(0, 1);
	sleep(100);

	for (uint8_t i = 1; i <= 80; i++)
	{
		floppy_seek(0, i);
		sleep(100);
	}

	sleep(5000);

	log("Turning off drive 0 motor...\n");
	floppy_set_motor(0, 0);*/

	log("Getting sector 0...\n");
	floppy_read_sector(0);
}
