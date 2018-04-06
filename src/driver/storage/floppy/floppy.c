#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/storage/floppy.h>

#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/kheap.h>
#include <kernel/interrupts/irqs.h>

static bool irqTriggered = false;
static bool implied_seeks = false;

extern bool floppy_init_dma();

/**
 * Handles IRQ6 firings
 */
static void floppy_callback(IrqRegisters_t* regs, uint8_t irq) {
	// Set our trigger value.
	irqTriggered = true;
}

/**
 * Waits for IRQ6 to be raised.
 * @return True if the IRQ was triggered; otherwise false if it timed out.
 */
bool floppy_wait_for_irq(uint16_t timeout) {
	// Wait until IRQ is triggered or we time out.
	uint8_t ret = false;
	while (!irqTriggered) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?
	if(irqTriggered)
		ret = true;
	else
		kprintf("FLOPPY: IRQ timeout!\n");

	// Reset triggered value.
	irqTriggered = false;
	return ret;
}


/**
 * Write a byte to the floppy controller
 * @param cmd The byte to write.
 */
void floppy_write_data(uint8_t data) {
	for (uint16_t i = 0; i < FLOPPY_IRQ_WAIT_TIME; i++) {
		// Wait until register is ready.
		if (inb(FLOPPY_REG_MSR) & FLOPPY_MSR_RQM) {
			outb(FLOPPY_REG_FIFO, data);
			return;
		}
		sleep(10);
	}
	kprintf("FLOPPY: Data timeout!\n");
}

/**
 * Read a byte from the floppy controller
 * @return The byte read. If a timeout occurs, 0.
 */
uint8_t floppy_read_data() {
	for (uint16_t i = 0; i < FLOPPY_IRQ_WAIT_TIME; i++) {
		// Wait until register is ready.
		if (inb(FLOPPY_REG_MSR) & FLOPPY_MSR_RQM)
			return inb(FLOPPY_REG_FIFO);
		sleep(10);
	}
	kprintf("FLOPPY: Data timeout!\n");
	return 0;
}

/**
 * Gets any interrupt data.
 * @param st0 Pointer to ST0 value.
 * @param cyl Pointer to cyl value.
 */
void floppy_sense_interrupt(uint8_t* st0, uint8_t* cyl) {
	// Send command and get result.
	floppy_write_data(FLOPPY_CMD_SENSE_INTERRUPT);
	*st0 = floppy_read_data();
	*cyl = floppy_read_data();
}

/**
 * Sets drive data.
 */
static void floppy_set_drive_data(uint8_t stepRate, uint16_t loadTime, uint8_t unloadTime, bool dma) {
	kprintf("FLOPPY: Setting drive info...\n");

	// Send specify command.
	floppy_write_data(FLOPPY_CMD_SPECIFY);
	uint8_t data = ((stepRate & 0xF) << 4) | (unloadTime & 0xF);
	floppy_write_data(data);
	data = (loadTime << 1) | dma ? 0 : 1;
	floppy_write_data(data);
}

/**
 * Sets drive motor state.
 */
void floppy_set_motor(uint8_t drive, bool on) {
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

/**
 * Detects floppy drives in CMOS.
 * @return True if drives were found; otherwise false.
 */
static bool floppy_detect() {
	kprintf("FLOPPY: Detecting drives from CMOS...\n");
	uint8_t a, b, c;
	outb(0x70, 0x10);
	c = inb(0x71);

	a = c >> 4; // Get high nibble
	b = c & 0xF; // Get low nibble by ANDing out low nibble

	char *drive_type[6] = { "no floppy drive", "360kb 5.25in floppy drive", 
	"1.2mb 5.25in floppy drive", "720kb 3.5in", "1.44mb 3.5in", "2.88mb 3.5in"};
	kprintf("FLOPPY: Drive A: %s\nFLOPPY: Drive B: %s\n", drive_type[a], drive_type[b]);

	// Did we find any drives?
	return (a > 0 || b > 0);
}

/**
 * Gets the version of the floppy controller.
 * @return The version byte.
 */
uint8_t floppy_version() {
	// Send version command and get version.
	floppy_write_data(FLOPPY_CMD_VERSION);
	return floppy_read_data();
}

/**
 * Resets the floppy controller.
 */
static void floppy_reset() {
	kprintf("FLOPPY: Resetting controller...\n");

	// Disable and re-enable floppy controller.
	outb(FLOPPY_REG_DOR, 0x00);
	outb(FLOPPY_REG_DOR, FLOPPY_DOR_IRQ_DMA | FLOPPY_DOR_RESET);
	floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);

	// Clear any interrupts on drives.
	uint8_t st0, cyl;
	for(int i = 0; i < 4; i++)
		floppy_sense_interrupt(&st0, &cyl);
}

// Configure default values.
// EIS - No Implied Seeks.
// EFIFO - FIFO Disabled.
// POLL - Polling Enabled.
// FIFOTHR - FIFO Threshold Set to 1 Byte.
// PRETRK - Pre-Compensation Set to Track 0.
static void floppy_configure(bool eis, bool efifo, bool poll, uint8_t fifothr, uint8_t pretrk) {
	kprintf("FLOPPY: Configuring controller...\n");

	// Send configure command.
	floppy_write_data(FLOPPY_CMD_CONFIGURE);
	floppy_write_data(0x00);
	uint8_t data = (!eis << 6) | (!efifo << 5) | (poll << 4) | fifothr;
	floppy_write_data(data);
	floppy_write_data(pretrk);

	// Lock configuration.
	floppy_write_data(FLOPPY_CMD_LOCK);
}

static void floppy_set_transfer_speed(uint8_t speed) {
	// Write speed to CCR.
	kprintf("FLOPPY: Setting transfer speed to 0x%X.\n", speed);
	outb(FLOPPY_REG_CCR, speed & 0x3);
}

/**
 * Recalibrates the drive.
 * @param drive The drive to recalibrate.
 */
static bool floppy_recalibrate(uint8_t drive) {
	uint8_t st0, cyl;
	if (drive >= 4)
		return false;

	// Turn on motor and attempt to calibrate.
	kprintf("FLOPPY: Calibrating drive %u...\n", drive);
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++)
	{
		// Send calibrate command.
		floppy_write_data(FLOPPY_CMD_RECALIBRATE);
		floppy_write_data(drive);
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// If current cylinder is zero, we are done.
		sleep(500);
		if (!cyl) {
			kprintf("FLOPPY: Calibration of drive %u passed!\n", drive);
			return true;
		}
	}

	// If we got here, calibration failed.
	kprintf("FLOPPY: Calibration of drive %u failed!\n", drive);
	return false;
}

/**
 * Initializes the floppy driver.
 */
void floppy_init() {
	kprintf("FLOPPY: Initializing...\n");

	// Detect drives.
	if (!floppy_detect()) {
		kprintf("FLOPPY: No drives found in CMOS. Aborting.\n");
		return;
	}

	// Install hander for IRQ6.
	irqs_install_handler(FLOPPY_IRQ, floppy_callback);

	// Reset controller and get version.
	floppy_reset();
	uint8_t version = floppy_version();

	// If version is 0xFF, that means there isn't a floppy controller.
	if (version == FLOPPY_VERSION_NONE) {
		kprintf("FLOPPY: No floppy controller present. Aborting.\n");
		return;
	}

	// Print version.
	kprintf("FLOPPY: Version: 0x%X.\n", version);

	// Initialize DMA.
	if (!floppy_init_dma()) {
		kprintf("FLOPPY: Failed to initialize DMA. Aborting.\n");
		return;
	}

	// Configure controller and reset it.
	implied_seeks = (version == FLOPPY_VERSION_ENHANCED);
	floppy_configure(implied_seeks, true, false, 0, 0);
	floppy_reset();

	// Set drive info (step time = 4ms, load time = 16ms, unload time = 240ms).
	floppy_set_transfer_speed(FLOPPY_DRATE_500KBPS);
	floppy_set_drive_data(0xC, 0x2, 0xF, true);

	// Calibrate first drive and switch off motor.
	floppy_recalibrate(0);
	floppy_set_motor(0, false);

	uint8_t *data = kheap_alloc(FLOPPY_DMALENGTH);
	kprintf("Getting track 0...\n");
	floppy_read_track(0, 0, data, FLOPPY_DMALENGTH);
	floppy_set_motor(0, false);

	for (int i = 0; i < 60; i++)
		kprintf("%X ", data[i]);
	kprintf("\n");

	char volumeName[12];
	memcpy(volumeName, data+43, sizeof(volumeName));
	volumeName[11] = '\0';

	// Print volume name.
	kprintf("FAT12 volume string: %s\n", volumeName);
	kprintf("FLOPPY: Initialized!\n");
}
