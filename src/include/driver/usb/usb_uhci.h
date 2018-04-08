#ifndef USB_UHCI_H
#define USH_UHCI_H

#include <main.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

#define USB_UHCI_PCI_REG_RELEASE_NUM    0x60

#define USB_UHCI_USBCMD(port)           (uint16_t)(port + 0x00)
#define USB_UHCI_USBSTS(port)           (uint16_t)(port + 0x02)
#define USB_UHCI_USBINTR(port)          (uint16_t)(port + 0x04)
#define USB_UHCI_FRNUM(port)            (uint16_t)(port + 0x06)
#define USB_UHCI_FRBASEADD(port)        (uint16_t)(port + 0x08)
#define USB_UHCI_SOFMOD(port)           (uint16_t)(port + 0x0C)
#define USB_UHCI_PORTSC1(port)          (uint16_t)(port + 0x10)
#define USB_UHCI_PORTSC2(port)          (uint16_t)(port + 0x12)

// All data is in 16 4KB pages.
#define USB_UHCI_FRAME_COUNT            1024
#define USB_UHCI_TD_POOL_SIZE           PAGE_SIZE_4K
#define USB_UHCI_QH_POOL_SIZE           PAGE_SIZE_4K

// Port status/control bits.
enum {
    USB_UHCI_PORT_STATUS_PRESENT        = 0x0001,
    USB_UHCI_PORT_STATUS_PRESENT_CHANGE = 0x0002,
    USB_UHCI_PORT_STATUS_ENABLED        = 0x0004,
    USB_UHCI_PORT_STATUS_ENABLE_CHANGE  = 0x0008,
    USB_UHCI_PORT_STATUS_RESUME_DETECT  = 0x0040,
    USB_UHCI_PORT_STATUS_LOW_SPEED      = 0x0100,
    USB_UHCI_PORT_STATUS_RESET          = 0x0200,
    USB_UHCI_PORT_STATUS_SUSPEND        = 0x1000
};

// UHCI frame list bits.
enum {
    USB_UHCI_FRAME_TERMINATE            = 0x01,
    USB_UHCI_FRAME_QUEUE_HEAD           = 0x02,
    USB_UHCI_FRAME_DEPTH_FIRST          = 0x04
};

enum {
    USB_UHCI_TD_STATUS_BITSTUFF_ERROR   = 0x020000,
    USB_UHCI_TD_STATUS_CRC_ERROR        = 0x040000,
    USB_UHCI_TD_STATUS_NAK              = 0x080000,
    USB_UHCI_TD_STATUS_BABBLE           = 0x100000,
    USB_UHCI_TD_STATUS_DATA_ERROR       = 0x200000,
    USB_UHCI_TD_STATUS_STALLED          = 0x400000,
    USB_UHCI_TD_STATUS_ACTIVE           = 0x800000,
    USB_UHCI_TD_STATUS_INTERRUPT_COMP   = 0x1000000,
    USB_UHCI_TD_STATUS_ISO_SELECT       = 0x2000000,
    USB_UHCI_TD_STATUS_LOW_SPEED        = 0x4000000,
    USB_UHCI_TD_STATUS_SHORT_PACKET     = 0x20000000
};

enum {
    USB_UHCI_STATUS_RUN                 = 0x0001,
    USB_UHCI_STATUS_RESET               = 0x0002,
    USB_UHCI_STATUS_GLOBAL_RESET        = 0x0004,
    USB_UHCI_STATUS_GLOBAL_SUSPEND      = 0x0008,
    USB_UHCI_STATUS_FORCE_GLOBAL_RESUME = 0x0010,
    USB_UHCI_STATUS_DEBUG               = 0x0020,
    USB_UHCI_STATUS_CONFIGURE           = 0x0040,
    USB_UHCI_STATUS_64_BYTE_PACKETS     = 0x0080
};

#define USB_UHCI_TD_PACKET_IN                    0x69
#define USB_UHCI_TD_PACKET_OUT                   0xe1
#define USB_UHCI_TD_PACKET_SETUP                 0x2d

// Transfer descriptors. Must be aligned to 16 bytes and a multiple of 16 bytes in size.
typedef struct {
    // UHCI fields. 16 bytes.
    uint32_t LinkPointer;

    // Control and status.
    uint16_t ActualLength : 11;
    uint32_t Reserved2 : 6;
    bool BitstuffError : 1;
    bool CrcError : 1;
    bool NakReceived : 1;
    bool BabbleDetected : 1;
    bool DataBufferError : 1;
    bool Stalled : 1;
    bool Active : 1;
    bool InterruptOnComplete : 1;
    bool IsochronousSelect : 1;
    bool LowSpeedDevice : 1;
    uint16_t ErrorCounter : 2;
    bool ShortPacketDetect : 1;
    uint16_t Reserved1 : 2;

    // Token.
    uint8_t PacketIdentification;
    uint8_t DeviceAddress : 7;
    uint8_t Endpoint : 4;
    bool DataToggle : 1;
    uint8_t Reserved3 : 1;
    uint16_t MaximumLength : 11;

    uint32_t BufferPointer;

    // Used to determine if this descriptor is in-use or not when allocating.
    uint8_t InUse; // 1 bytes.
    uint32_t NextDesc; // 4 bytes;
    uint8_t pad[11]; // 11 bytes.
} __attribute__((packed)) usb_uhci_transfer_desc_t;

// Queue heads. Must be aligned to 16 bytes and be a multiple of 16 bytes in size.
typedef struct {
    // UHCI fields. 8 bytes.
    uint32_t Head;
    uint32_t Element;

    uint8_t InUse; // 1 byte.
    uint32_t TransferDescHead; // 4 bytes;

    uint32_t PreviousQueueHead; // 4 bytes.
    uint32_t NextQueueHead; // 4 bytes.
    uint8_t pad[11]; // 11 bytes.
} __attribute__((packed)) usb_uhci_queue_head_t;

typedef struct {
    PciDevice *PciDevice;
    uint32_t BaseAddress;
    uint8_t SpecVersion;
    uint32_t *FrameList;

    usb_uhci_transfer_desc_t *TransferDescPool;
    usb_uhci_queue_head_t *QueueHeadPool;
    usb_uhci_queue_head_t *QueueHead;
} usb_uhci_controller_t;

#endif
