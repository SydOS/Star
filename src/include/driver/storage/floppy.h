/*
 * File: floppy.h
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

#ifndef FLOPPY_H
#define FLOPPY_H

#include <main.h>

#define FLOPPY_IRQ  6

// Floppy registers.
enum {
    FLOPPY_REG_SRA  = 0x3F0, // Status Register A, read-only.
    FLOPPY_REG_SRB  = 0x3F1, // Status Register B, read-only.
    FLOPPY_REG_DOR  = 0x3F2, // Digital Output Register, read-write.
    FLOPPY_REG_TDR  = 0x3F3, // Tape Drive Register, read-write.
    FLOPPY_REG_MSR  = 0x3F4, // Main Status Register, read-only.
    FLOPPY_REG_DSR  = 0x3F4, // Data Rate Select Register, write-only.
    FLOPPY_REG_FIFO = 0x3F5, // Data (FIFO), read-write.
    FLOPPY_REG_DIR  = 0x3F7, // Digital Input Register, read-only.
    FLOPPY_REG_CCR  = 0x3F7  // Configuration Control Register, write-only.
};

// Floppy commands.
enum {
    FLOPPY_CMD_READ_TRACK           = 0x02, // Read track.
    FLOPPY_CMD_SPECIFY              = 0x03, // Set drive parameters.
    FLOPPY_CMD_SENSE_DRIVE_STATUS   = 0x04, // Get drive information.
    FLOPPY_CMD_WRITE_DATA           = 0x05, // Write sector.
    FLOPPY_CMD_READ_DATA            = 0x06, // Read sector.
    FLOPPY_CMD_RECALIBRATE          = 0x07, // Calibrate and seek to sector 0.
    FLOPPY_CMD_SENSE_INTERRUPT      = 0x08, // Get status of last command.
    FLOPPY_CMD_WRITE_DELETED_DATA   = 0x09, // Write sector with deleted flag.
    FLOPPY_CMD_READ_ID              = 0x0A,	// Get current position of recording heads.
    FLOPPY_CMD_READ_DELETED_DATA    = 0x0C, // Read sector with deleted flag.
    FLOPPY_CMD_FORMAT_TRACK         = 0x0D, // Formats a track.
    FLOPPY_CMD_DUMPREG              = 0x0E, // Diagnostics dump.
    FLOPPY_CMD_SEEK                 = 0x0F, // Seek to track.
    FLOPPY_CMD_VERSION              = 0x10,	// Get controller versison.
    FLOPPY_CMD_SCAN_EQUAL           = 0x11, // Scan for data equal.
    FLOPPY_CMD_PERPENDICULAR_MODE   = 0x12,	// Used before using commands that access a perpendicular disk drive.
    FLOPPY_CMD_CONFIGURE            = 0x13, // Set controller parameters.
    FLOPPY_CMD_LOCK                 = 0x14, // Locks in controller parameters so a reset does not affect them.
    FLOPPY_CMD_VERIFY               = 0x16, // Verify sector.
    FLOPPY_CMD_SCAN_LOW_OR_EQUAL    = 0x19, // Scan for data less or equal.
    FLOPPY_CMD_SCAN_HIGH_OR_EQUAL   = 0x1D, // Scan for data greater or equal.
    FLOPPY_CMD_RELATIVE_SEEK        = 0x8F  // Seek to relative sector.
};

// Floppy command options.
enum {
    FLOPPY_CMD_EXT_SKIP     = 0x20, // Skip flag. When set to 1, sectors containing a deleted data address
                                    // mark will automatically be skipped during the execution of READ DATA.
    FLOPPY_CMD_EXT_MFM      = 0x40, // MFM mode selector. A one selects the double density (MFM) mode.
    FLOPPY_CMD_EXT_MT       = 0x80  // Multi-track selector. When set, this flag selects the multi-track operating mode.
};

// Floppy DOR bits.
enum {
    FLOPPY_DOR_SEL_DRIVE0   = 0x00, // Select drive 0.
    FLOPPY_DOR_SEL_DRIVE1   = 0x01, // Select drive 1.
    FLOPPY_DOR_SEL_DRIVE2   = 0x02, // Select drive 2.
    FLOPPY_DOR_SEL_DRIVE3   = 0x03, // Select drive 3.
    FLOPPY_DOR_RESET        = 0x04, // Clear to enter reset mode, set to operate normally.
    FLOPPY_DOR_IRQ_DMA      = 0x08, // Enable IRQs and DMA.
    FLOPPY_DOR_MOT_DRIVE0   = 0x10, // Set to turn drive 0's motor on.
    FLOPPY_DOR_MOT_DRIVE1   = 0x20, // Set to turn drive 1's motor on.
    FLOPPY_DOR_MOT_DRIVE2   = 0x40, // Set to turn drive 2's motor on.
    FLOPPY_DOR_MOT_DRIVE3   = 0x80  // Set to turn drive 3's motor on.
};

// Floppy MSR bits.
enum {
    FLOPPY_MSR_DRIVE0_BUSY  = 0x01, // Drive 0 is seeking.
    FLOPPY_MSR_DRIVE1_BUSY  = 0x02, // Drive 1 is seeking.
    FLOPPY_MSR_DRIVE2_BUSY  = 0x04, // Drive 2 is seeking.
    FLOPPY_MSR_DRIVE3_BUSY  = 0x08, // Drive 3 is seeking.
    FLOPPY_MSR_CMD_BUSY     = 0x10, // This bit is set to a one when a command is in progress. Cleared at end of result phase.
    FLOPPY_MSR_NON_DMA      = 0x20, // This mode is selected in the SPECIFY command and will be set to a 1 during the execution phase of a command.
    FLOPPY_MSR_DIO          = 0x40, // Indicates the direction of a data transfer once RQM is set. A 1 indicates a read and a 0 indicates a write is required.
    FLOPPY_MSR_RQM          = 0x80, // Indicates that the host can transfer data if set to a 1. No access is permitted if set to a 0.
};

// Floppy ST0 masks.
enum {
    FLOPPY_ST0_SEL_DRIVE0       = 0x00, // Drive 0 is selected.
    FLOPPY_ST0_SEL_DRIVE1       = 0x01, // Drive 1 is selected.
    FLOPPY_ST0_SEL_DRIVE2       = 0x02, // Drive 2 is selected.
    FLOPPY_ST0_SEL_DRIVE3       = 0x03, // Drive 3 is selected.
    FLOPPY_ST0_ACTIVE_HEAD      = 0x04, // The current head address.
    FLOPPY_ST0_FAIL             = 0x08, // Drive not ready.
    FLOPPY_ST0_SEEK_END         = 0x10, // The 82077AA completed a SEEK or RECALIBRATE command, or a READ or WRITE with implied seek command.
    FLOPPY_ST0_INTERRUPT_CODE   = 0xC0  // Command failed.
};

// Floppy ST1 masks.
enum {
    FLOPPY_ST1_MISSING_ADDR_MARK    = 0x01, // No address mark found.
    FLOPPY_ST1_NOT_WRITABLE         = 0x02, // Disk is write-protected.
    FLOPPY_ST1_NO_DATA              = 0x04, // Sector not found.
    FLOPPY_ST1_OVERRUN_UNDERRUN     = 0x10, // Becomes set if the 82077AA does not receive CPU or DMA Underrun service
                                            // within the required time interval, resulting in data overrun or underrun.
    FLOPPY_ST1_DATA_ERROR           = 0x20, // The 82077AA detected a CRC error in either the ID field or the data field of a sector.
    FLOPPY_ST1_END_OF_CYLINDER      = 0x80  // The 82077AA tried to access a sector beyond the final sector of the track.
};

// Floppy ST2 masks.
enum {
    FLOPPY_ST2_MISSING_DATA_MARK    = 0x01, // The 82077AA cannot detect a data address mark or a deleted data address mark.
    FLOPPY_ST2_BAD_CYLINDER         = 0x02, // The track address from the sector ID field is different from the track address
                                            // maintained inside the 82077AA and is equal to FF hex which indicates a bad track
                                            // with a hard error according to the IBM soft-sectored format.
    FLOPPY_ST2_WRONG_CYLINDER       = 0x10, // The track address from the sector ID field is different from the track address maintained inside the 82077AA.
    FLOPPY_ST2_DATA_ERROR_IN_FIELD  = 0x20, // The 82077AA detected a CRC error in the data field.
    FLOPPY_ST2_CONTROL_MARK         = 0x40  // Data address mark found.
};

// Floppy ST3 masks.
enum {
    FLOPPY_ST3_SEL_DRIVE0       = 0x00, // Indicates the status of the DS1, DS0 pins.
    FLOPPY_ST3_SEL_DRIVE1       = 0x01,
    FLOPPY_ST3_SEL_DRIVE2       = 0x02,
    FLOPPY_ST3_SEL_DRIVE3       = 0x03,
    FLOPPY_ST3_HEAD_ADDRESS     = 0x04, // Indicates the status of the HDSEL pin.
    FLOPPY_ST3_TRACK0           = 0x10, // Indicates the status of the TRK0 pin.
    FLOPPY_ST3_WRITE_PROTECT    = 0x40  // Indicates the status of the WP pin.
};

// Floppy bytes/sector values.
enum {
    FLOPPY_BYTES_SECTOR_128     = 0x00,
    FLOPPY_BYTES_SECTOR_256     = 0x01,
    FLOPPY_BYTES_SECTOR_512     = 0x02,
    FLOPPY_BYTES_SECTOR_1024    = 0x04
};

// Floppy sector gap sizes.
enum {
    FLOPPY_GAP3_STANDARD    = 0x2A,
    FLOPPY_GAP3_5_25        = 0x20,
    FLOPPY_GAP3_3_5         = 0x1B
};

// Floppy data rates.
enum {
    FLOPPY_DRATE_1MBPS      = 0x3,
    FLOPPY_DRATE_500KBPS    = 0x0,
    FLOPPY_DRATE_300KBPS    = 0x1,
    FLOPPY_DRATE_250KBPS    = 0x2
};

#define FLOPPY_CMD_RETRY_COUNT  10
#define FLOPPY_IRQ_WAIT_TIME    500
#define FLOPPY_DMALENGTH 0x4800
#define FLOPPY_SECTORS_PER_TRACK 18
#define FLOPPY_VERSION_NONE     0xFF
#define FLOPPY_VERSION_ENHANCED 0x90

extern int8_t floppy_read(uint8_t drive, uint32_t sectorLba, uint8_t buffer[], uint16_t bufferSize);

extern int8_t floppy_read_sector(uint8_t drive, uint32_t sectorLba, uint8_t buffer[], uint16_t bufferSize);
extern int8_t floppy_read_track(uint8_t drive, uint8_t track, uint8_t buffer[], uint16_t bufferSize);

extern void floppy_write_data(uint8_t data);
extern uint8_t floppy_read_data();
extern void floppy_sense_interrupt(uint8_t* st0, uint8_t* cyl);
extern void floppy_set_motor(uint8_t drive, bool on);
extern uint8_t floppy_version();
extern void floppy_init();

#endif
