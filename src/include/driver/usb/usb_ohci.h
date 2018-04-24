#ifndef USB_OHCI_H
#define USH_OHCI_H

#include <main.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

#include <driver/usb/devices/usb_device.h>

//
// Control and status.
//
#define USB_OHCI_REG_REVISION           0x00
#define USB_OHCI_REG_CONTROL            0x04
#define USB_OHCI_REG_COMMAND_STATUS     0x08
#define USB_OHCI_REG_INTERRUPT_STATUS   0x0C
#define USB_OHCI_REG_INTERRUPT_ENABLE   0x10
#define USB_OHCI_REG_INTERRUPT_DISABLE  0x14

// Revision register.
#define USB_OHCI_REG_REVISION_MASK      0xFF
#define USB_OHCI_MIN_VERSION            0x10

// Control register.
#define USB_OHCI_REG_CONTROL_CBSR_MASK      0xFF
#define USB_OHCI_REG_CONTROL_PERIOD_ENABLE  0x04
#define USB_OHCI_REG_CONTROL_ISO_ENABLE     0x08
#define USB_OHCI_REG_CONTROL_CONTROL_ENABLE 0x10
#define USB_OHCI_REG_CONTROL_BULK_ENABLE    0x20

#define USB_OHCI_REG_CONTROL_STATE_USBRESET         0x00
#define USB_OHCI_REG_CONTROL_STATE_USBRESUME        0x40
#define USB_OHCI_REG_CONTROL_STATE_USBOPERATIONAL   0x80
#define USB_OHCI_REG_CONTROL_STATE_USBSUSPEND       0xC0

#define USB_OHCI_REG_CONTROL_INTERRUPT_ROUTING  0x100
#define USB_OHCI_REG_CONTROL_REMOTE_WAKEUP_CONN 0x200
#define USB_OHCI_REG_CONTROL_REMOTE_WAKEUP_EN   0x400

// Status register.
#define USB_OHCI_REG_STATUS_RESET                   0x01
#define USB_OHCI_REG_STATUS_CONTROL_LIST_FILLED     0x02
#define USB_OHCI_REG_STATUS_BULK_LIST_FILLED        0x04
#define USB_OHCI_REG_STATUS_OWNERSHIP_CHANGE_REQ    0x08


#define USB_OHCI_REG_HCCA               0x18
#define USB_OHCI_REG_PERIOD_CURRENT_ED  0x1C
#define USB_OHCI_REG_CONTROL_HEAD_ED    0x20
#define USB_OHCI_REG_CONTROL_CURRENT_ED 0x24
#define USB_OHCI_REG_BULK_HEAD_ED       0x28
#define USB_OHCI_REG_BULK_CURRENT_ED    0x2C
#define USB_OHCI_REG_DONE_HEAD          0x30

#define USB_OHCI_REG_FRAME_INTERVAL     0x34
#define USB_OHCI_REG_FRAME_REMAINING    0x38
#define USB_OHCI_REG_FRAME_NUMBER       0x3C
#define USB_OHCI_REG_PERIODIC_START     0x40
#define USB_OHCI_REG_LS_THRESHOLD       0x44

// Root hub info.
#define USB_OHCI_REG_RH_DESCRIPTOR_A    0x48
#define USB_OHCI_REG_RH_DESCRIPTOR_B    0x4C
#define USB_OHCI_REG_RH_STATUS          0x50
#define USB_OHCI_REG_RH_PORT_STATUS     0x54

#define USB_OHCI_REG_RH_GET_NUMPORTS(value) ((uint8_t)(value & 0xFFFF))
#define USB_OHCI_REG_RH_NO_POWER_SWITCHING  0x100

// Refer to 7.4.4 HcRhPortStatus[1:NDP] Register.
#define USB_OHCI_PORT_STATUS_CONNECTED              0x0001
#define USB_OHCI_PORT_STATUS_ENABLED                0x0002
#define USB_OHCI_PORT_STATUS_SUSPENDED              0x0004
#define USB_OHCI_PORT_STATUS_OVERCURRENT            0x0008
#define USB_OHCI_PORT_STATUS_RESET                  0x0010
#define USB_OHCI_PORT_STATUS_POWERED                0x0100
#define USB_OHCI_PORT_STATUS_LOWSPEED               0x0200
#define USB_OHCI_PORT_STATUS_CONNECT_CHANGED        0x1000
#define USB_OHCI_PORT_STATUS_ENABLE_CHANGED         0x2000
#define USB_OHCI_PORT_STATUS_SUSPEND_CHANGED        0x4000
#define USB_OHCI_PORT_STATUS_OVERCURRENT_CHANGED    0x8000
#define USB_OHCI_PORT_STATUS_RESET_CHANGED          0x10000

#define USB_OHCI_PORT_STATUS_DISABLE                0x0001
#define USB_OHCI_PORT_STATUS_ENABLE                 0x0002
#define USB_OHCI_PORT_STATUS_SUSPEND                0x0004
#define USB_OHCI_PORT_STATUS_RESUME                 0x0008
#define USB_OHCI_PORT_STATUS_POWERON                0x0100
#define USB_OHCI_PORT_STATUS_POWEROFF               0x0200


#define USB_OHCI_MEM_BLOCK_SIZE         8
#define USB_OHCI_ENDPOINT_POOL_SIZE           PAGE_SIZE_4K
#define USB_OHCI_TRANSFER_POOL_SIZE           PAGE_SIZE_4K
#define USB_OHCI_MEM_POOL_SIZE          (PAGE_SIZE_64K - USB_OHCI_ENDPOINT_POOL_SIZE - USB_OHCI_TRANSFER_POOL_SIZE)
#define USB_OHCI_MEM_BLOCK_COUNT        (USB_OHCI_MEM_POOL_SIZE / USB_OHCI_MEM_BLOCK_SIZE)


#define USB_OHCI_INTERRUPT_SCHEDULE_OVERRUN         0x01
#define USB_OHCI_INTERRUPT_DONE_HEAD_WRITEBACK      0x02
#define USB_OHCI_INTERRUPT_START_OF_FRAME           0x04
#define USB_OHCI_INTERRUPT_RESUME_DETECT            0x08
#define USB_OHCI_INTERRUPT_ERROR                    0x10
#define USB_OHCI_INTERRUPT_FRAME_NUMBER_OVERFLOW    0x20
#define USB_OHCI_INTERRUPT_ROOT_HUB_STATUS_CHANGE   0x40
#define USB_OHCI_INTERRUPT_OWNERSHIP_CHANGE         0x40000000
#define USB_OHCI_MASTER_INTERRUPT_ENABLE            0x80000000

#define USB_OHCI_DESC_HALTED    0x1

// Endpoint descriptor (16 bytes in size).
// See 4.2 Endpoint Descriptor in the OHCI guide.
typedef struct {
    // Dword 0.
    uint8_t FunctionAddress : 7;
    uint8_t EndpointNumber : 4;
    uint8_t Direction : 2;
    bool LowSpeed : 1;
    bool Skip : 1;
    bool IsochronousFormat : 1;
    uint16_t MaxPacketSize : 11;

    // Reserved for use by OS.
    uint8_t Reserved : 4;
    bool InUse : 1;

    // Dwords 1 to 3.
    uint32_t TailPointer;
    uint32_t HeadPointer;
    uint32_t NextEndpointDesc;
} __attribute__((packed)) usb_ohci_endpoint_desc_t;

#define USB_OHCI_TD_PACKET_SETUP    0x0
#define USB_OHCI_TD_PACKET_OUT      0x1
#define USB_OHCI_TD_PACKET_IN       0x2

#define USB_OHCI_TD_CONDITION_NOERROR           0x0
#define USB_OHCI_TD_CONDITION_CRC               0x1
#define USB_OHCI_TD_CONDITION_BITSTUFF          0x2
#define USB_OHCI_TD_CONDITION_TOGGLE_MISMATCH   0x3
#define USB_OHCI_TD_CONDITION_STALL             0x4
#define USB_OHCI_TD_CONDITION_NOT_RESPONDING    0x5
#define USB_OHCI_TD_CONDITION_PID_CHECK_FAILURE 0x6
#define USB_OHCI_TD_CONDITION_UNEXPECTED_PID    0x7
#define USB_OHCI_TD_CONDITION_DATA_OVERRUN      0x8
#define USB_OHCI_TD_CONDITION_DATA_UNDERRUN     0x9
#define USB_OHCI_TD_CONDITION_BUFFER_OVERRUN    0xC
#define USB_OHCI_TD_CONDITION_BUFFER_UNDERRUN   0xD
#define USB_OHCI_TD_CONDITION_NOT_ACCESSED      0xF

// Transfer descriptor (16 bytes in size, 32).
// See 4.3.1 General Transfer Descriptor in the OHCI guide.
typedef struct {
    // Dword 0.
    // Reserved for use by OS.
    uint32_t Reserved : 17;
    bool InUse : 1;

    bool BufferRounding : 1;
    uint8_t Direction : 2;
    uint8_t DelayInterrupt : 3;
    bool DataToggle : 1;
    bool NoToggleCarry : 1;
    uint8_t ErrorCount : 2;
    uint8_t ConditionCode : 4;

    // Dwords 1 to 3.
    uint32_t CurrentBufferPointer;
    uint32_t NextTransferDesc;
    uint32_t BufferEnd;

    // Additional info.
    uint32_t Next;
    uint32_t Pad[3];
} __attribute__((packed)) usb_ohci_transfer_desc_t;

// Isochronous transfer descriptor (32 bytes in size).
// See 4.3.2 Isochronous Transfer Descriptor in the OHCI guide.
typedef struct {
    // Dword 0.
    uint16_t StartingFrame : 16;
    
    // Reserved for use by OS.
    uint8_t Reserved1 : 5;
    uint8_t DelayInterrupt : 3;
    uint8_t FrameCount : 3;

    // Reserved for use by OS.
    bool InUse : 1;
    uint8_t ConditionCode : 4;

    // Dwords 1 to 3.
    uint32_t BufferPage0; 
    uint32_t NextTransferDesc;
    uint32_t BufferEnd;

   // Packet status words (dwords 4 to 7).
   uint16_t Psw0;
   uint16_t Psw1;
   uint16_t Psw2;
   uint16_t Psw3;
   uint16_t Psw4;
   uint16_t Psw5;
   uint16_t Psw6;
   uint16_t Psw7;
} __attribute__((packed)) usb_ohci_isochronous_transfer_desc_t;

// Host Controller Communications Area.
// See 4.4 Host Controller Communications Area in the OHCI guide.
typedef struct {
    uint32_t InterruptTable[32];
    uint16_t CurrentFrameNumber;
    uint16_t Pad1;
    uint32_t DoneHead;
    uint8_t Reserved[116];
} __attribute__((packed)) usb_ohci_controller_comm_area_t;

typedef struct {
    PciDevice *PciDevice;
    uint32_t BaseAddress;

    uint32_t *BasePointer;
    uintptr_t BaseAddressVirt;


    bool MemMap[USB_OHCI_MEM_BLOCK_COUNT];
    uint8_t *HeapPool;

    bool AddressPool[USB_MAX_DEVICES];
    usb_ohci_endpoint_desc_t *EndpointDescPool;
    usb_ohci_transfer_desc_t *TransferDescPool;


    usb_ohci_controller_comm_area_t *Hcca;


    uint16_t NumPorts;
    usb_device_t *RootDevice;
} usb_ohci_controller_t;

#endif
