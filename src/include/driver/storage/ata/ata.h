/*
 * File: ata.h
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

#ifndef ATA_H
#define ATA_H

#include <main.h>
#include <driver/pci.h>
#include <kernel/memory/paging.h>

// Primary PATA interface ports.
#define ATA_PRI_COMMAND_PORT    0x1F0
#define ATA_PRI_CONTROL_PORT    0x3F4

// Secondary PATA interface ports.
#define ATA_SEC_COMMAND_PORT    0x170
#define ATA_SEC_CONTROL_PORT    0x374

// Command register offsets.
#define ATA_REG_DATA(port)                  (port+0x0) // Read/write PIO data bytes.
#define ATA_REG_ERROR(port)                 (port+0x1) // Contains status or error from the last command.
#define ATA_REG_FEATURES(port)              (port+0x1) // Used for ATAPI.
#define ATA_REG_SECTOR_COUNT(port)          (port+0x2) // Number of sectors to read/write.
#define ATA_REG_SECTOR_NUMBER(port)         (port+0x3) // CHS, LBA.
#define ATA_REG_LBA_LOW(port)               (port+0x3) // LBA low bits.
#define ATA_REG_CYLINDER_LOW(port)          (port+0x4) // Low part of sector address.
#define ATA_REG_LBA_MID(port)               (port+0x4) // LBA middle bits.
#define ATA_REG_CYLINDER_HIGH(port)         (port+0x5) // High part of sector address.
#define ATA_REG_LBA_HIGH(port)              (port+0x5) // LBA high bits.
#define ATA_REG_DRIVE_SELECT(port)          (port+0x6) // Selects the drive and/or head.
#define ATA_REG_COMMAND(port)               (port+0x7) // Send commands or read status.
#define ATA_REG_STATUS(port)                (port+0x7) // Read status.

// Control register offset.
#define ATA_REG_DEVICE_CONTROL(port)        (port+0x2)
#define ATA_REG_ALT_STATUS(port)            (port+0x2)

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

enum {
    ATA_DEVICE_FLAGS_SLAVE          = 0x10,
    ATA_DEVICE_FLAGS_SECTOR         = 0x20,
    ATA_DEVICE_FLAGS_LBA            = 0x40,
    ATA_DEVICE_FLAGS_ECC            = 0x80
};

// ATA device signature.
#define ATA_SIG_SECTOR_COUNT_ATA    0x01
#define ATA_SIG_SECTOR_NUMBER_ATA   0x01
#define ATA_SIG_CYLINDER_LOW_ATA    0x00
#define ATA_SIG_CYLINDER_HIGH_ATA   0x00

// ATAPI device signature.
#define ATA_SIG_SECTOR_COUNT_ATAPI  0x01
#define ATA_SIG_SECTOR_NUMBER_ATAPI 0x01
#define ATA_SIG_CYLINDER_LOW_ATAPI  0x14
#define ATA_SIG_CYLINDER_HIGH_ATAPI 0xEB

#define ATA_SECTOR_SIZE_512         512
#define ATA_SECTOR_SIZE_4K          4096

// Values returned from ata_check_status().
#define ATA_CHK_STATUS_OK               0
#define ATA_CHK_STATUS_DRIVE_BUSY       -1
#define ATA_CHK_STATUS_DEVICE_FAULT     -2
#define ATA_CHK_STATUS_DEVICE_MISMATCH  -10
#define ATA_CHK_STATUS_ERROR            1



// PCI ATA controller bits
#define ATA_PCI_PIF_PRI_NATIVE_MODE         0x01
#define ATA_PCI_PIF_PRI_SUPPORTS_NATIVE     0x02
#define ATA_PCI_PIF_SEC_NATIVE_MODE         0x04
#define ATA_PCI_PIF_SEC_SUPPORTS_NATIVE     0x08
#define ATA_PCI_PIF_BUSMASTER               0x80

#define ATA_PCI_STATUS_INTERRUPT            0x08

#define ATA_PCI_BUSMASTER_PORT_PRICMD       0x00
#define ATA_PCI_BUSMASTER_PORT_PRISTATUS    0x02
#define ATA_PCI_BUSMASTER_PORT_PRIPRDT      0x04
#define ATA_PCI_BUSMASTER_PORT_SECCMD       0x08
#define ATA_PCI_BUSMASTER_PORT_SECSTATUS    0x0A
#define ATA_PCI_BUSMASTER_PORT_SECPRDT      0x0C

#define ATA_PCI_BUSMASTER_STATUS_INTERRUPT  0x04

// ATA DMA PRD.
typedef struct {
    uint32_t BufferAddress;
    uint16_t ByteCount;

    uint16_t Reserved : 15;
    bool EndOfTable : 1;
} __attribute__((packed)) ata_prd_t;

// Sufficient for a transfer of 256 512-byte sectors.
#define ATA_PRD_COUNT       32
#define ATA_PRD_BUF_SIZE    PAGE_SIZE_4K

#define ATA_DMA_CMD_START   (1 << 0)
#define ATA_DMA_CMD_WRITE   (1 << 3)

struct ata_device_t;

// ATA channel.
typedef struct {
    uint16_t CommandPort;
    uint16_t ControlPort;
    uint8_t Interrupt;
    bool InterruptTriggered;

    bool BusMasterCapable;
    uint16_t BusMasterCommandPort;
    uint16_t BusMasterStatusPort;
    uint16_t BusMasterPrdt;

    uint32_t PrdtPage;
    ata_prd_t *Prdt;
    uint8_t *PrdBuffers[ATA_PRD_COUNT];

    struct ata_device_t *MasterDevice;
    struct ata_device_t *SlaveDevice;
} ata_channel_t;

// ATA device.
typedef struct {
    ata_channel_t *Channel;
    bool Master;

    uint16_t BytesPerSector;
    bool Lba48BitSupported;
} ata_device_t;

// ATA controller.
typedef struct {
    ata_channel_t *Primary;
    ata_channel_t *Secondary;
} ata_controller_t;

typedef struct {
    bool Error : 1;
    uint8_t Unused : 2;
    bool DataRequest : 1;
    bool ServiceRequest : 1;
    bool DriveFault : 1;
    bool Ready : 1;
    bool Busy : 1;
} __attribute__((packed)) ata_reg_status_t;

#include <driver/storage/ata/ata_commands.h>

extern int16_t ata_check_status(ata_channel_t *channel, bool master);
extern void ata_read_data_pio(ata_channel_t *channel, uint32_t size, void *outData, uint32_t length);
extern void ata_write_data_pio(ata_channel_t *channel, const void *data, uint32_t size);
extern void ata_send_params(ata_channel_t *channel, uint8_t sectorCount, uint8_t sectorNumber, uint8_t cylinderLow, uint8_t cylinderHigh);
extern void ata_send_command(ata_channel_t *channel, uint8_t sectorCount, uint8_t sectorNumber, uint8_t cylinderLow, uint8_t cylinderHigh, uint8_t command);
extern void ata_set_lba_high(ata_channel_t *channel, uint8_t lbaHigh);
extern void ata_select_device(ata_channel_t *channel, bool master);
extern bool ata_wait_for_drq(ata_channel_t *channel);


extern void ata_dma_reset(ata_channel_t *channel);
extern void ata_dma_start(ata_channel_t *channel, bool write);

extern bool ata_init(pci_device_t *pciDevice);

#endif
