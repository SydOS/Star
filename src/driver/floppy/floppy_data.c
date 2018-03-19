#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/floppy.h>

#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/kheap.h>

// Buffer for DMA transfers.
static uint8_t *floppyDmaBuffer;

extern bool floppy_wait_for_irq(uint16_t timeout);

/**
 * Initializes floppy DMA.
 */
bool floppy_init_dma() {
    // Allocate space for DMA buffer.
	uintptr_t frame = 0;
	if (!pmm_dma_get_free_frame(&frame))
		return false;
	floppyDmaBuffer = (uint8_t*)frame;

    // Clear out DMA buffer.
    memset(floppyDmaBuffer, 0, PAGE_SIZE_64K);

	// Determine address and length of buffer.
	union {
		uint8_t bytes[4];
		uint32_t data;
	} addr, count;
	addr.data = (uint32_t)pmm_dma_get_phys((uintptr_t)floppyDmaBuffer);
	count.data = PAGE_SIZE_64K - 1;

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

	// Unmask DMA channel 2.
	outb(0x0A, 0x02);
	return true;
}

static void floppy_set_dma(bool write) {
	// Mask DMA channel 2, send mode, and unmask DMA channel 2.
	outb(0x0A, 0x06);
	outb(0x0B, write ? 0x5A : 0x56);
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
		floppy_write_data(FLOPPY_CMD_SEEK);
		floppy_write_data((0 << 2) | drive); // Head 0, drive.
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

		// Initialize DMA.
		floppy_set_dma(false);

		// Send read command to disk.
		kprintf("Attempting to read head %u, track %u, sector %u...\n", head, track, sector);
		floppy_write_data(FLOPPY_CMD_READ_DATA | FLOPPY_CMD_EXT_SKIP | FLOPPY_CMD_EXT_MFM | FLOPPY_CMD_EXT_MT);
		floppy_write_data(head << 2 | drive);
		floppy_write_data(track);
		floppy_write_data(head);
		floppy_write_data(sector);
		floppy_write_data(FLOPPY_BYTES_SECTOR_512);
		floppy_write_data(1);
		floppy_write_data(FLOPPY_GAP3_3_5);
		floppy_write_data(0xFF);

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
			memcpy(buffer, floppyDmaBuffer, bufferSize);
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

	// Initialize DMA.
	floppy_set_dma(false);

	// Attempt to read sector.
	floppy_set_motor(drive, true);
	for (uint8_t i = 0; i < FLOPPY_CMD_RETRY_COUNT; i++) {
		sleep(100);

		// Send read command to disk.
		kprintf("Attempting to read track %u...\n", track);
		floppy_write_data(FLOPPY_CMD_READ_DATA | FLOPPY_CMD_EXT_MFM);
		floppy_write_data(0 << 2 | drive);
		floppy_write_data(track);
		floppy_write_data(0);
		floppy_write_data(1); // First sector.
		floppy_write_data(FLOPPY_BYTES_SECTOR_512);
		floppy_write_data(18); // read 18 sectors.
		floppy_write_data(FLOPPY_GAP3_3_5);
		floppy_write_data(0xFF);

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
			memcpy(buffer, floppyDmaBuffer, bufferSize);
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