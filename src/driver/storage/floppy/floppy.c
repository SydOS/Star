/*
 * File: floppy.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/storage/floppy.h>

#include <kernel/storage/storage.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/kheap.h>
#include <kernel/interrupts/irqs.h>

static bool irqTriggered = false;
static bool implied_seeks = false;

extern bool floppy_init_dma();

static char *driveTypes[6] = { "no floppy drive", "360KB 5.25\" floppy drive", 
	"1.2MB 5.25\" floppy drive", "720KB 3.5\"", "1.44MB 3.5\"", "2.88MB 3.5\""};

/**
 * Handles IRQ6 firings
 */
static bool floppy_callback(irq_regs_t* regs, uint8_t irq) {
	// Set our trigger value.
	irqTriggered = true;
	return true;
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
void floppy_set_drive_data(uint8_t stepRate, uint16_t loadTime, uint8_t unloadTime, bool dma) {
	// Send specify command.
	floppy_write_data(FLOPPY_CMD_SPECIFY);
	uint8_t data = ((stepRate & 0xF) << 4) | (unloadTime & 0xF);
	floppy_write_data(data);
	data = (loadTime << 1) | dma ? 0 : 1;
	floppy_write_data(data);
}

static inline uint8_t floppy_get_motor_num(floppy_drive_t *floppyDrive) {
	// Get mask based on drive passed.
	switch (floppyDrive->Number) {
		case 0:
			return FLOPPY_DOR_MOT_DRIVE0;

		case 1:
			return FLOPPY_DOR_MOT_DRIVE1;

		case 2:
			return FLOPPY_DOR_MOT_DRIVE2;

		case 3:
			return FLOPPY_DOR_MOT_DRIVE3;

		default:
			return -1;
	}
}

bool floppy_motor_on(floppy_drive_t *floppyDrive) {
	uint8_t motor = floppy_get_motor_num(floppyDrive);
	if (motor == -1)
		return false;

	// Turn motor on or off and wait 500ms for motor to spin up.
	outb(FLOPPY_REG_DOR, FLOPPY_DOR_RESET | FLOPPY_DOR_IRQ_DMA | floppyDrive->Number | motor);
	sleep(500);
	return true;
}

bool floppy_motor_off(floppy_drive_t *floppyDrive) {
	uint8_t motor = floppy_get_motor_num(floppyDrive);
	if (motor == -1)
		return false;

	// Turn motor off.
	outb(FLOPPY_REG_DOR, FLOPPY_DOR_RESET | FLOPPY_DOR_IRQ_DMA);
	return true;
}

/**
 * Detects floppy drives in CMOS.
 * @return True if drives were found; otherwise false.
 */
static bool floppy_detect(uint8_t *outTypeA, uint8_t *outTypeB) {
	kprintf("FLOPPY: Detecting drives from CMOS...\n");
	outb(0x70, 0x10);
	uint8_t types = inb(0x71);

	// Parse drives.
	*outTypeA = types >> 4; // Get high nibble.
	*outTypeB = types & 0xF; // Get low nibble by ANDing out low nibble.

	// Did we find any drives?
	if (*outTypeA > FLOPPY_TYPE_2880_35 || *outTypeB > FLOPPY_TYPE_2880_35)
		return false;
	kprintf("FLOPPY: Drive A: %s\nFLOPPY: Drive B: %s\n", driveTypes[*outTypeA], driveTypes[*outTypeB]);
	return (*outTypeA > 0 || *outTypeB > 0);
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

void floppy_set_transfer_speed(uint8_t driveType) {
	// Determine speed.
	uint8_t speed = FLOPPY_SPEED_500KBPS;
	switch (driveType) {
		// TODO
	}

	// Write speed to CCR.
	outb(FLOPPY_REG_CCR, speed & 0x3);
}

/**
 * Recalibrates a drive.
 * @param floppyDrive	The drive to recalibrate.
 */
static bool floppy_recalibrate(floppy_drive_t *floppyDrive) {
	uint8_t st0, cyl;
	if (floppyDrive->Number >= 4)
		return false;

	// Turn on motor and attempt to calibrate.
	kprintf("FLOPPY: Calibrating drive %u...\n", floppyDrive->Number);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++)
	{
		// Send calibrate command.
		floppy_write_data(FLOPPY_CMD_RECALIBRATE);
		floppy_write_data(floppyDrive->Number);
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);
		floppy_sense_interrupt(&st0, &cyl);

		// If current cylinder is zero, we are done.
		sleep(500);
		if (!cyl) {
			kprintf("FLOPPY: Calibration of drive %u passed!\n", floppyDrive->Number);
			return true;
		}
	}

	// If we got here, calibration failed.
	kprintf("FLOPPY: Calibration of drive %u failed!\n", floppyDrive->Number);
	return false;
}

static void floppy_dma_set(uint8_t *buffer, uint32_t length, bool write) {
	// Determine address and length of buffer.
	union {
		uint8_t bytes[4];
		uint32_t data;
	} addr, count;
	addr.data = (uint32_t)pmm_dma_get_phys((uintptr_t)buffer);
	count.data = length - 1;

	// Ensure address is under 24 bits, and count is under 16 bits.
	if ((addr.data >> 24) || (count.data >> 16) || (((addr.data & 0xFFFF) + count.data) >> 16))
		panic("FLOPPY: Invalid DMA buffer location!\n");

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

	// Send read/write mode.
	outb(0x0B, write ? 0x5A : 0x56);

	// Unmask DMA channel 2.
	outb(0x0A, 0x02);
}

// Convert LBA to CHS.
static void floppy_lba_to_chs(uint32_t lba, uint16_t* cyl, uint16_t* head, uint16_t* sector) {
	*cyl = lba / (2 * FLOPPY_SECTORS_PER_TRACK);
	*head = ((lba % (2 * FLOPPY_SECTORS_PER_TRACK)) / FLOPPY_SECTORS_PER_TRACK);
	*sector = ((lba % (2 * FLOPPY_SECTORS_PER_TRACK)) % FLOPPY_SECTORS_PER_TRACK + 1);
}

// Parse and print errors.
static uint8_t floppy_parse_error(uint8_t st0, uint8_t st1, uint8_t st2) {
	if (st0 & FLOPPY_ST0_INTERRUPT_CODE || st1 > 0 || st2 > 0)
		kprintf("FLOPPY: Error status ST0: 0x%X  ST1: 0x%X  ST2: 0x%X\n", st0, st1, st2);

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
	if (st1 & FLOPPY_ST1_MISSING_ADDR_MARK || st2 & FLOPPY_ST2_MISSING_DATA_MARK) {
		kprintf("Missing address mark.\n");
		error = 1;
	}
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

// Seek to specified track.
bool floppy_seek(floppy_drive_t *floppyDrive, uint8_t track) {	
	uint8_t st0, cyl = 0;
	if (floppyDrive->Number >= 4)
		return false;

	// Attempt seek.
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		// Send seek command.
		kprintf("Seeking to track %u...\n", track);
		floppy_write_data(FLOPPY_CMD_SEEK);
		floppy_write_data((0 << 2) | floppyDrive->Number); // Head 0, drive.
		floppy_write_data(track);

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
	kprintf("Seek failed for %u on drive %u!\n", track, floppyDrive->Number);
	return false;
}

int8_t floppy_read_track(floppy_drive_t *floppyDrive, uint16_t track) {
	// Set drive info (step time = 4ms, load time = 16ms, unload time = 240ms).
	floppy_set_transfer_speed(floppyDrive->Type);
	floppy_set_drive_data(0xC, 0x2, 0xF, true);

	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		// Initialize DMA.
		uint32_t dmaLength = 2 * 18 * 512;
		floppy_dma_set(floppyDrive->DmaBuffer, dmaLength, false);

		// Send read command to disk to read both sides of track.
		kprintf("Attempting to read track %u...\n", track);
		floppy_write_data(FLOPPY_CMD_READ_DATA | FLOPPY_CMD_EXT_SKIP | FLOPPY_CMD_EXT_MFM | FLOPPY_CMD_EXT_MT);
		floppy_write_data(0 << 2 | floppyDrive->Number);
		floppy_write_data(track); 	// Track.
		floppy_write_data(0); 		// Head 0.
		floppy_write_data(1);		// Start at sector 1.
		floppy_write_data(FLOPPY_BYTES_SECTOR_512);
		floppy_write_data(18);		// 18 sectors per track?
		floppy_write_data(FLOPPY_GAP3_3_5);
		floppy_write_data(0xFF);

		// Wait for IRQ.
		floppy_wait_for_irq(FLOPPY_IRQ_WAIT_TIME);

		// Get status registers.
		uint8_t st0 = floppy_read_data();
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
			return 0;
		}
		else if (error > 1) {
			kprintf("Not retrying...\n");
			return 2;
		}

		sleep(100);
	}

	// Failed.
	kprintf("Get sector failed!\n");
	return - 1;
}

bool floppy_read_blocks(floppy_drive_t *floppyDrive, const uint64_t *blocks, uint32_t blockSize, uint32_t blockCount, uint8_t *outBuffer, uint32_t length) {
	if (floppyDrive->Number >= 4)
		return false;

	// Turn on motor.
	floppy_motor_on(floppyDrive);

	// Get each block.
	uint32_t remainingLength = length;
	uint32_t bufferOffset = 0;
	uint16_t lastTrack = -1;
	for (uint32_t block = 0; block < blockCount; block++) {
		for (uint32_t sectorOffset = 0; sectorOffset < blockSize; sectorOffset++) {
			uint32_t sectorLba = (blocks[block] / 512) + sectorOffset;

			// Convert LBA to CHS.
			uint16_t head = 0, track = 0, sector = 1;
			floppy_lba_to_chs(sectorLba, &track, &head, &sector);

			// Have we changed tracks?.
			if (lastTrack != track) {
				if (lastTrack != track && !floppy_seek(floppyDrive, track)) {
					floppy_motor_off(floppyDrive);
					return false;
				}

				// Get track.
				lastTrack = track;
				floppy_read_track(floppyDrive, track);
			}
			
			uint32_t size = remainingLength;
			if (size > 512)
				size = 512;

			uint32_t headOffset = head == 1 ? (18 * 512) : 0;
			memcpy(outBuffer + bufferOffset, floppyDrive->DmaBuffer + ((sector - 1) * 512) + headOffset, size);

			//int8_t error = floppy_read_chs(drive, track, head, sector, outBuffer+bufferOffset, size);
			bufferOffset += 512;
		}
	}

	floppy_motor_off(floppyDrive);
	return true;
}

static bool floppy_storage_read(storage_device_t *storageDevice, uint64_t startByte, uint8_t *outBuffer, uint32_t length) {
	return floppy_read_blocks((floppy_drive_t*)storageDevice->Device, &startByte, 1, 1, outBuffer, length);
}

static bool floppy_storage_read_blocks(storage_device_t *storageDevice, const uint64_t *blocks, uint32_t blockSize, uint32_t blockCount, uint8_t *outBuffer, uint32_t length) {
	return floppy_read_blocks((floppy_drive_t*)storageDevice->Device, blocks, blockSize, blockCount, outBuffer, length);
}

static void floppy_drive_init(floppy_drive_t *floppyDrive) {
	// Set drive info (step time = 4ms, load time = 16ms, unload time = 240ms).
	floppy_set_transfer_speed(floppyDrive->Type);
	floppy_set_drive_data(0xC, 0x2, 0xF, true);

	// Calibrate drive and switch off motor.
	floppy_motor_on(floppyDrive);
	floppy_recalibrate(floppyDrive);
	floppy_motor_off(floppyDrive);
}

/**
 * Initializes the floppy driver.
 */
bool floppy_init(void) {
	kprintf("\e[93mFLOPPY: Initializing...\n");

	// Detect drives.
	uint8_t driveTypeA, driveTypeB = 0;
	if (!floppy_detect(&driveTypeA, &driveTypeB)) {
		kprintf("FLOPPY: No drives found in CMOS. Aborting.\e[0m\n");
		return false;
	}

	// Install hander for IRQ6.
	irqs_install_handler(FLOPPY_IRQ, floppy_callback);

	// Reset controller and get version.
	floppy_reset();
	uint8_t version = floppy_version();

	// If version is 0xFF, that means there isn't a floppy controller.
	if (version == FLOPPY_VERSION_NONE) {
		kprintf("FLOPPY: No floppy controller present. Aborting.\e[0m\n");
		return false;
	}

	// Print version.
	kprintf("FLOPPY: Version: 0x%X.\n", version);

	// Allocate space for DMA buffer.
	uintptr_t frame = 0;
	if (!pmm_dma_get_free_frame(&frame)) {
		kprintf("FLOPPY: Failed to initialize DMA. Aborting.\e[0m\n");
		return false;
	}

    // Clear out DMA buffer.
    memset((uint8_t*)frame, 0, PAGE_SIZE_64K);

	// Configure and reset controller.
	floppy_configure(false, true, false, 0, 0);
	floppy_reset();

	// Initialize drive A if present.
	if (driveTypeA) {
		// Create drive object.
		kprintf("FLOPPY: Initializing drive A (%s)...\n", driveTypes[driveTypeA]);
		floppy_drive_t *driveA = (floppy_drive_t*)kheap_alloc(sizeof(floppy_drive_t));
		memset(driveA, 0, sizeof(floppy_drive_t));
		
		// Set drive settings.
		driveA->BaseAddress = 0;
		driveA->Number = 0;
		driveA->Version = version;
		driveA->Type = driveTypeA;
		driveA->DmaBuffer = (uint8_t*)frame;

		// Initialize drive.
		floppy_drive_init(driveA);

		// Register storage device.
		storage_device_t *floppyStorageDevice = (storage_device_t*)kheap_alloc(sizeof(storage_device_t));
		memset(floppyStorageDevice, 0, sizeof(storage_device_t));
		floppyStorageDevice->Device = driveA;

		floppyStorageDevice->Read = floppy_storage_read;
		floppyStorageDevice->ReadBlocks = floppy_storage_read_blocks;
		storage_register(floppyStorageDevice);
	}

	kprintf("FLOPPY: Initialized!\e[0m\n");
}
