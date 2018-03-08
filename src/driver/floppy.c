#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/floppy.h>
#include <arch/i386/kernel/interrupts.h>

static bool irq_triggered = false;
static bool implied_seeks = false;

// Buffer for DMA transfers.
static uint8_t floppyDmaBuffer[FLOPPY_DMALENGTH]
	__attribute__((aligned(0x8000)));

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
	kprintf("Floppy drive A: %s\nFloppy drive B: %s\n", drive_type[a], drive_type[b]);
}

// Parse and print errors.
static uint8_t floppy_parse_error(uint8_t st0, uint8_t st1, uint8_t st2) {
	uint8_t error = 0;
	if (st0 & FLOPPY_ST0_INTERRUPT_CODE) {
		static const char *status[] = { 0, "command did not complete", "invalid command", "polling error" };
		kprintf("An error occurred while getting the sector: %s.\n", status[st0 >> 6]);
		error = 1;
	}
	if (st0 & FLOPPY_ST0_FAIL) {
		kprintf("Drive not ready.\n");
		error = 1;
	}
	//if (st1 & FLOPPY_ST1_MISSING_ADDR_MARK || st2 & FLOPPY_ST2_MISSING_DATA_MARK) {
	//	kprintf("Missing address mark.\n");
	//	error = 1;
	//}
	if (st1 & FLOPPY_ST1_NOT_WRITABLE) {
		kprintf("Disk is write-protected.\n");
		error = 2;
	}
	if (st1 & FLOPPY_ST1_NO_DATA) {
		kprintf("Sector not found.\n");
		error = 1;
	}
	if (st1 & FLOPPY_ST1_OVERRUN_UNDERRUN) {
		kprintf("Buffer overrun/underrun.\n");
		error = 1;
	}
	if (st1 & FLOPPY_ST1_DATA_ERROR) {
		kprintf("CRC error.\n");
		error = 1;
	}
	if (st1 & FLOPPY_ST1_END_OF_CYLINDER) {
		kprintf("End of track.\n");
		error = 1;
	}
	if (st2 & FLOPPY_ST2_BAD_CYLINDER) {
		kprintf("Bad track.\n");
		error = 1;
	}
	if (st2 & FLOPPY_ST2_WRONG_CYLINDER) {
		kprintf("Wrong track.\n");
		error = 1;
	}
	if (st2 & FLOPPY_ST2_DATA_ERROR_IN_FIELD) {
		kprintf("CRC error in data.\n");
		error = 1;
	}
	if (st2 & FLOPPY_ST2_CONTROL_MARK) {
		kprintf("Deleted address mark.\n");
		error = 1;
	}

	return error;
}

// Handle firings IRQ6.
static void floppy_irq_handler(registers_t* regs) {
	irq_triggered = true;
	kprintf("IRQ6 raised!\n");
}

// Wait for IRQ6 to be raised.
static bool floppy_wait_for_irq(uint16_t timeout) {
	uint8_t ret = false;
	while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(10);
	}
	if(irq_triggered) { ret = true; } else { kprintf("FLOPPY DRIVE IRQ TIMEOUT!\n"); }
	irq_triggered = false;
	return ret;
}

// Init DMA. write = true to write, write = false to read.
static void floppy_dma_init(bool write) {
	union {
		uint8_t bytes[4];
		uint32_t length;
	} addr, count;

	addr.length = (unsigned)&floppyDmaBuffer;
	count.length = (unsigned)FLOPPY_DMALENGTH - 1;

	// Ensure address is under 24 bits, and count is under 16 bits.
	if ((addr.length >> 24) || (count.length >> 16) || (((addr.length & 0xFFFF) + count.length) >> 16)) {
		kprintf("INVALID FLOPPY DMA BUFFER SIZE/LOCATION!\n");
		return;
		// Kernel should die here.
	}

	// https://wiki.osdev.org/ISA_DMA#The_Registers.
	// Mask DMA channel 2 and reset flip-flop.
	outb(0x0A, 0x06);
	outb(0x0C, 0xFF);

	// Send address and page register.
	outb(0x04, addr.bytes[0]);
	outb(0x04, addr.bytes[1]);
	outb(0x81, addr.bytes[2]);

	// Reset flip-flop and send count.
	outb(0x0C, 0xFF);
	outb(0x05, count.bytes[0]);
	outb(0x05, count.bytes[1]);

	// Send mode and unmask DMA channel 2.
	outb(0x0B, write ? 0x5A : 0x56);
	outb(0x0A, 0x02);
}

// Convert LBA to CHS.
static void floppy_lba_to_chs(uint32_t lba, uint16_t* cyl, uint16_t* head, uint16_t* sector) {
	*cyl = lba / (2 * FLOPPY_SECTORS_PER_TRACK);
	*head = ((lba % (2 * FLOPPY_SECTORS_PER_TRACK)) / FLOPPY_SECTORS_PER_TRACK);
	*sector = ((lba % (2 * FLOPPY_SECTORS_PER_TRACK)) % FLOPPY_SECTORS_PER_TRACK + 1);
}

// Send command to floppy controller.
static void floppy_write_command(uint8_t cmd) {
	for (uint16_t i = 0; i < FLOPPY_IRQ_WAIT_TIME; i++) {
		// Wait until register is ready.
		if (inb(FLOPPY_REG_MSR) & FLOPPY_MSR_RQM) {
			outb(FLOPPY_REG_FIFO, cmd);
			return;
		}
		sleep(10);
	}
	kprintf("FLOPPY DRIVE DATA TIMEOUT!\n");
}

// Read data from FIFO register.
static uint8_t floppy_read_data() {
	for (uint16_t i = 0; i < FLOPPY_IRQ_WAIT_TIME; i++) {
		// Wait until register is ready.
		if (inb(FLOPPY_REG_MSR) & FLOPPY_MSR_RQM)
			return inb(FLOPPY_REG_FIFO);
		sleep(10);
	}
	kprintf("FLOPPY DRIVE DATA TIMEOUT!\n");
	return 0;
}

// Get any interrupts from last command.
static void floppy_sense_interrupt(uint8_t* st0, uint8_t* cyl) {
	floppy_write_command(FLOPPY_CMD_SENSE_INTERRUPT);
	*st0 = floppy_read_data();
	*cyl = floppy_read_data();
}

// Sets drive data.
static void floppy_set_drive_data(uint8_t step_rate, uint16_t load_time, uint8_t unload_time, bool dma) {
	// Send specify command.
	floppy_write_command(FLOPPY_CMD_SPECIFY);
	uint8_t data = ((step_rate & 0xF) << 4) | (unload_time & 0xF);
	floppy_write_command(data);
	data = (load_time << 1) | dma ? 0 : 1;
	floppy_write_command(data);
}

// Sets motor state.
static void floppy_set_motor(uint8_t drive, bool on) {
	// Check drive.
	if (drive >= 4)
		return;

	// Get mask based on drive passed.
	uint8_t motor = 0;
	switch (drive) {
		case 0:
			motor = FLOPPY_DOR_MOT_DRIVE0;
			break;

		case 1:
			motor = FLOPPY_DOR_MOT_DRIVE1;
			break;

		case 2:
			motor = FLOPPY_DOR_MOT_DRIVE2;
			break;

		case 3:
			motor = FLOPPY_DOR_MOT_DRIVE3;
			break;
	}

	// Turn motor on or off and wait 500ms for motor to spin up.
	outb(FLOPPY_REG_DOR, FLOPPY_DOR_RESET | FLOPPY_DOR_IRQ_DMA | (on ? (drive | motor) : 0));
	sleep(500);
}

// Recalibrate drive to track 0.
static bool floppy_recalibrate(uint8_t drive) {
	uint8_t st0, cyl;
	if (drive >= 4)
		return false;

	// Turn on motor and attempt to calibrate.
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++)
	{
		// Send calibrate command.
		floppy_write_command(FLOPPY_CMD_RECALIBRATE);
		floppy_write_command(drive);
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// If current cylinder is zero, we are done.
		if (!cyl)
			return true;
	}

	// If we got here, calibration failed.
	kprintf("Calibration of drive %u failed!\n", drive);
	return false;
}

// Reset floppy controller.
static void floppy_controller_reset(bool full) {
	// Disable and re-enable floppy controller.
	outb(FLOPPY_REG_DOR, 0x00);
	outb(FLOPPY_REG_DOR, FLOPPY_DOR_IRQ_DMA | FLOPPY_DOR_RESET);
	floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);

	// Clear any interrupts on drives.
	uint8_t st0, cyl;
	for(int i = 0; i < 4; i++)
		floppy_sense_interrupt(&st0, &cyl);

	// Are we doing a full reset (parameters + calibration)?
	if (!full)
		return;

	// Set transfer speed to 500 kb/s.
	kprintf("Setting transfer speed...\n");
	outb(FLOPPY_REG_CCR, 0x00);

	// Set drive info (step time = 4ms, load time = 16ms, unload time = 240ms).
	kprintf("Setting floppy drive info...\n");
	floppy_set_drive_data(0xC, 0x2, 0xF, true);

	// Calibrate first drive.
	kprintf("Calibrating floppy drive...\n");
	floppy_recalibrate(0);
	sleep(1000);
}

// Configure default values.
// EIS - No Implied Seeks.
// EFIFO - FIFO Disabled.
// POLL - Polling Enabled.
// FIFOTHR - FIFO Threshold Set to 1 Byte.
// PRETRK - Pre-Compensation Set to Track 0.
static void floppy_configure(bool eis, bool efifo, bool poll, uint8_t fifothr, uint8_t pretrk) {
	// Send configure command.
	floppy_write_command(FLOPPY_CMD_CONFIGURE);
	floppy_write_command(0x00);
	uint8_t data = (!eis << 6) | (!efifo << 5) | (poll << 4) | fifothr;
	floppy_write_command(data);
	floppy_write_command(pretrk);

	// Lock configuration.
	floppy_write_command(FLOPPY_CMD_LOCK);
}

// Seek to specified track.
bool floppy_seek(uint8_t drive, uint8_t track) {	
	uint8_t st0, cyl = 0;
	if (drive >= 4)
		return false;

	// Attempt seek.
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		// Send seek command.
		kprintf("Seeking to track %u...\n", track);
		floppy_write_command(FLOPPY_CMD_SEEK);
		floppy_write_command((0 << 2) | drive); // Head 0, drive.
		floppy_write_command(track);

		// Wait for response and check interrupt.
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// Ensure command completed successfully.
		if (st0 & FLOPPY_ST0_INTERRUPT_CODE) {
			kprintf("Error executing floppy seek command!\n");
			continue;
		}
		
		// If we have reached the requested track, return.
		if (cyl == track) {
			sleep(500);
			return true;
		}		
	}

	// Seek failed if we get here.
	kprintf("Seek failed for %u on drive %u!\n", track, drive);
	return false;
}

// Read the specified sector.
int8_t floppy_read_sector(uint8_t drive, uint32_t sectorLba, uint8_t buffer[], uint16_t bufferSize) {
	if (drive >= 4)
		return -1;
	uint8_t st0, cyl = 0;

	// Convert LBA to CHS.
	uint16_t head = 0, track = 0, sector = 1;
	floppy_lba_to_chs(sectorLba, &head, &track, &sector);

	// Seek to track.	
	if (!floppy_seek(drive, track))
		return -1;

	// Attempt to read sector.
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		sleep(100);

		// Send read command to disk.
		kprintf("Attempting to read head %u, track %u, sector %u...\n", head, track, sector);
		floppy_write_command(FLOPPY_CMD_READ_DATA | FLOPPY_CMD_EXT_SKIP | FLOPPY_CMD_EXT_MFM | FLOPPY_CMD_EXT_MT);
		floppy_write_command(head << 2 | drive);
		floppy_write_command(track);
		floppy_write_command(head);
		floppy_write_command(sector);
		floppy_write_command(FLOPPY_BYTES_SECTOR_512);
		floppy_write_command(1);
		floppy_write_command(FLOPPY_GAP3_3_5);
		floppy_write_command(0xFF);

		// Wait for IRQ.
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// Get status registers.
		st0 = floppy_read_data();
		uint8_t st1 = floppy_read_data();
		uint8_t st2 = floppy_read_data();
		uint8_t rTrack = floppy_read_data();
		uint8_t rHead = floppy_read_data();
		uint8_t rSector = floppy_read_data();
		uint8_t bytesPerSector = floppy_read_data();

		// Determine errors if any.
		uint8_t error = floppy_parse_error(st0, st1, st2);

		// If no error, we are done.
		if (!error) {
			// Copy data to buffer.
			memcpy(floppyDmaBuffer, buffer, bufferSize);
			return 0;
		}
		else if (error > 1) {
			kprintf("Not retrying...\n");
			return 2;
		}
	}

	// Failed.
	kprintf("Get sector failed!\n");
	return - 1;
}

// Read the specified track.
int8_t floppy_read_track(uint8_t drive, uint8_t track, uint8_t buffer[], uint16_t bufferSize) {
	if (drive >= 4)
		return -1;
	uint8_t st0, cyl = 0;

	// Seek to track.	
	if (!floppy_seek(drive, track))
		return -1;

	// Attempt to read sector.
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		sleep(100);

		// Send read command to disk.
		kprintf("Attempting to read track %u...\n", track);
		floppy_write_command(FLOPPY_CMD_READ_DATA | FLOPPY_CMD_EXT_MFM);
		floppy_write_command(0 << 2 | drive);
		floppy_write_command(track);
		floppy_write_command(0);
		floppy_write_command(1); // First sector.
		floppy_write_command(FLOPPY_BYTES_SECTOR_512);
		floppy_write_command(18); // read 18 sectors.
		floppy_write_command(FLOPPY_GAP3_3_5);
		floppy_write_command(0xFF);

		// Wait for IRQ.
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// Get status registers.
		st0 = floppy_read_data();
		uint8_t st1 = floppy_read_data();
		uint8_t st2 = floppy_read_data();
		uint8_t rTrack = floppy_read_data();
		uint8_t rHead = floppy_read_data();
		uint8_t rSector = floppy_read_data();
		uint8_t bytesPerSector = floppy_read_data();

		// Determine errors if any.
		uint8_t error = floppy_parse_error(st0, st1, st2);

		// If no error, we are done.
		if (!error) {
			// Copy data to buffer.
			memcpy(floppyDmaBuffer, buffer, bufferSize);
			return 0;
		}
		else if (error > 1) {
			kprintf("Not retrying...\n");
			return 2;
		}
	}

	// Failed.
	kprintf("Get track failed!\n");
	return - 1;
}

// Initializes floppy driver.
void floppy_init()
{
	// Install IRQ6 handler.
	interrupts_irq_install_handler(6, floppy_irq_handler);

	// Initialize DMA.
	floppy_dma_init(false);

	// Reset controller and get version.
	floppy_controller_reset(false);
	floppy_write_command(FLOPPY_CMD_VERSION);
	uint8_t version = floppy_read_data();

	// If version is 0xFF, that means there isn't a floppy controller.
	if (version == FLOPPY_VERSION_NONE)
	{
		kprintf("No floppy controller present, aborting!\n");
		return;
	}

	// Print version.
	kprintf("Floppy controller version is 0x%X!\n", version);

	// Set and lock base config.
	kprintf("Configuring floppy drive controller...\n");
	implied_seeks = (version == FLOPPY_VERSION_ENHANCED);
	floppy_configure(implied_seeks, true, false, 0, 0);

	// Reset floppy controller.
	kprintf("Resetting floppy drive controller...\n");
	floppy_controller_reset(true);
	floppy_set_motor(0, false);

	/*uint8_t data[16000];
	kprintf("Getting sector 0...\n");
	floppy_read_track(0, 0, data, 16000);
	floppy_set_motor(0, false);

	for (int i = 0; i < 60; i++)
		kprintf("%X ", data[i]);
	kprintf("\n");

	char volumeName[12];
	memcpy(data+43, volumeName, sizeof(volumeName));
	volumeName[11] = '\0';

	// Print volume name.
	kprintf("FAT12 volume string: %s\n", volumeName);*/
}
