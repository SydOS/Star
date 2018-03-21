#ifndef ATA_H
#define ATA_H

#include <main.h>

// Primary PATA interface ports.
#define ATA_PRI_COMMAND_PORT    0x1F0
#define ATA_PRI_CONTROL_PORT    0x3F4

// Secondary PATA interface ports.
#define ATA_SEC_COMMAND_PORT    0x170
#define ATA_SEC_CONTROL_PORT    0x374

// Command register offsets.
#define ATA_REG_DATA(port)                  (port+0x00) // Read/write PIO data bytes.
#define ATA_REG_ERROR(port)                 (port+0x01) // Contains status or error from the last command.
#define ATA_REG_FEATURES(port)              (port+0x01) // Used for ATAPI.
#define ATA_REG_SECTOR_COUNT(port)          (port+0x02) // Number of sectors to read/write.
#define ATA_REG_SECTOR_NUMBER(port)         (port+0x03) // CHS, LBA.
#define ATA_REG_CYLINDER_LOW(port)          (port+0x04) // Low part of sector address.
#define ATA_REG_CYLINDER_HIGH(port)         (port+0x05) // High part of sector address.
#define ATA_REG_DRIVE_SELECT(port)          (port+0x06) // Selects the drive and/or head.
#define ATA_REG_COMMAND(port)               (port+0x07) // Send commands or read status.
#define ATA_REG_STATUS(port)                (port+0x07) // Read status.

// Control register offset.
#define ATA_REG_DEVICE_CONTROL(port)        (port+0x02)
#define ATA_REG_ALT_STATUS(port)            (port+0x02)

// Device status bits.
enum {
    ATA_STATUS_ERROR                = 0x01, // Error condition present.
    ATA_STATUS_SENSE_DATA           = 0x02,
    ATA_STATUS_ALIGN_ERROR          = 0x04,
    ATA_STATUS_DATA_REQUEST         = 0x08,
    ATA_STATUS_DEFERRED_WRITE_ERROR = 0x10,
    ATA_STATUS_STREAM_ERROR         = 0x20,
    ATA_STATUS_DRIVE_FAULT          = 0x20,
    ATA_STATUS_READY                = 0x40, // Device is ready.
    ATA_STATUS_BUSY                 = 0x80  // Device is busy.
};

// Device control register bits.
enum {
    ATA_DEVICE_CONTROL_NO_INT       = 0x02,
    ATA_DEVICE_CONTROL_RESET        = 0x04,
    ATA_DEVICE_CONTROL_HIGH_BYTE    = 0x80
};


extern void ata_init();

#endif
