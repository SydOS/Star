/*
 * File: ahci.h
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

#ifndef AHCI_H
#define AHCI_H

#include <main.h>
#include <driver/pci.h>
#include <driver/storage/ahci/ahci_port.h>

// Generic host control registers.
#define AHCI_REG_HOST_CAP                   0x00
#define AHCI_REG_GLOBAL_HOST_CONTROL        0x04
#define AHCI_REG_INTERUPT_STATUS            0x08
#define AHCI_REG_PORTS_IMPLEMENTED          0x0C
#define AHCI_REG_VERSION                    0x10
#define AHCI_REG_CMD_COMPLETIION_CO_CONTROL 0x14
#define AHCI_REG_CMD_COMPLETITON_CO_PORTS   0x18
#define AHCI_REG_ENCLOSURE_MGMT_LOCATION    0x1C
#define AHCI_REG_ENCLOSURE_MGMT_CONTROL     0x20
#define AHCI_REG_HOST_CAP_EXTENDED          0x24
#define AHCI_REG_BIOS_HANDOFF               0x28

// Port registers.
#define AHCI_REG_PORT_OFFSET                0x100
#define AHCI_REG_PORT_MULT                  0x80

#define AHCI_REG_PORT_COMMAND_LIST          0x00
#define AHCI_REG_PORT_FIS                   0x08
#define AHCI_REG_PORT_INTERRUPT_STATUS      0x10
#define AHCI_REG_PORT_INTERRUPT_ENABLE      0x14
#define AHCI_REG_PORT_COMMAND_STATUS        0x18
#define AHCI_REG_PORT_TASK_FILE_DATA        0x20
#define AHCI_REG_PORT_SIGNATURE             0x24
#define AHCI_REG_PORT_SATA_STATUS           0x28
#define AHCI_REG_PORT_SATA_CONTROL          0x2C
#define AHCI_REG_PORT_SATA_ERROR            0x30
#define AHCI_REG_PORT_SATA_ACTIVE           0x34
#define AHCI_REG_PORT_COMMAND_ISSUE         0x38
#define AHCI_REG_PORT_SATA_NOTIFICATION     0x3C
#define AHCI_REG_PORT_FIS_CONTROL           0x40
#define AHCI_REG_PORT_DEVICE_SLEEP          0x44
#define AHCI_REG_PORT_VENDOR_SPECIFIC       0x70


typedef struct {
    // Number of ports.
    uint8_t PortCount : 5;

    // One or more eSATA ports present.
    bool ExternalCapable : 1;

    bool EnclosureManagementSupported : 1;
    bool CommandCompletionCoalescingSupported : 1;
    uint8_t CommandSlotCount : 5;

    bool PartialStateCapable : 1;
    bool SlumberStateCapable : 1;
    bool MultiplePioDrqBlocksSupported : 1;
    bool FisSwitchingSupported : 1;
    bool PortMultiplierSupported : 1;
    bool AhciOnlySupported : 1;
    bool Reserved : 1;

    uint8_t MaxSpeed : 4;

    bool CommandListOverrideSupported : 1;
    bool ActivityLedSupported : 1;
    bool AggressiveLinkPowerManagementSupported : 1;
    bool StaggeredSpinupSupported : 1;
    bool MechanicalPresenceSwitchSupported : 1;
    bool SNotificationSupported : 1;
    bool NativeCommandQueuingSupported : 1;
    bool Addressing64bitSupported : 1;
} __attribute__((packed)) ahci_host_cap_t;

// Offset 0x04, Global HBA Control.
typedef struct {
    bool HbaReset : 1;
    bool InterruptsEnabled : 1;
    bool MsiRevertToSingleMessage : 1;

    uint32_t Reserved : 28;
    bool AhciEnabled : 1;
} __attribute__((packed)) ahci_global_control_t;

// Offset 0x10, AHCI Version.
typedef struct {
    uint16_t Minor;
    uint16_t Major;
} __attribute__((packed)) ahci_version_t;

// Offset 0x14, Command Completion Coalescing Control.
typedef struct {
    bool Enabled : 1;
    uint8_t Reserved : 2;
    uint8_t Interrupt : 5;

    uint8_t CommandCompletions;
    uint16_t TimeoutValue;
} __attribute__((packed)) ahci_ccc_control_t;

// Offset 0x1C, Enclosure Management Location.
typedef struct {
    uint16_t BufferSize;
    uint16_t Offset;
} __attribute__((packed)) ahci_enc_mgmt_location_t;

// Offset 0x20, Enclosure Management Control.
typedef struct {
    bool MessageReceived : 1;
    uint8_t Reserved1 : 7;

    bool TransmitMessage : 1;
    bool Reset : 1;
    uint8_t Reserved2 : 6;

    bool LedMessageTypesSupported : 1;
    bool SafTeMessagesSupported : 1;
    bool Ses2MessagesSupported : 1;
    bool SgpioMessagesSupported : 1;
    uint8_t Reserved3 : 4;

    bool SingleMessageBuffer : 1;
    bool TransmitOnly : 1;
    bool ActivityLedHardwareDriven : 1;
    bool PortMultiplierSupported : 1;
} __attribute__((packed)) ahci_enc_mgmt_control_t;

// Offset 0x24, HBA Capabilities Extended.
typedef struct {
    bool Handoff : 1;
    bool NvmhciPresent : 1;
    bool AutoPartialToSlumber : 1;
    bool DeviceSleepSupported : 1;
    bool AggressiveSleepManagementSupported : 1;
    bool DeviceSleepFromSlumberOnly : 1;
    uint32_t Reserved : 26;
} __attribute__((packed)) ahci_hba_cap_extended_t;

typedef struct {
    bool BiosOwnedSemaphore : 1;
    bool OsOwnedSemaphore : 1;
    bool SmiOnOwnershipChange : 1;
    bool OsOwnershipChange : 1;
    bool BiosBusy : 1;
    uint32_t Reserved : 27;
} __attribute__((packed)) ahci_handoff_state_t;

typedef struct {
    ahci_host_cap_t Capabilities;
    ahci_global_control_t GlobalControl;
    uint32_t InterruptStatus;
    uint32_t PortsImplemented;

    ahci_version_t Version;
    ahci_ccc_control_t CccControl;
    uint32_t CccPorts;

    ahci_enc_mgmt_location_t EnclosureManagementLocation;
    ahci_enc_mgmt_control_t EnclosureManagementControl;
    ahci_hba_cap_extended_t CapabilitiesExtended;
    ahci_handoff_state_t Handoff;

    uint8_t Reserved[0x100-0x2C];
    ahci_port_memory_t Ports[32];
} __attribute__((packed)) ahci_memory_t;

// Command List Structure.
typedef struct {
    uint8_t CommandFisLength : 5;
    bool Atapi : 1;
    bool Write : 1;
    bool Prefetchable : 1;
    bool Reset : 1;
    bool Bist : 1;
    bool ClearBusyUponOk : 1;
    bool Reserved1 : 1;

    uint8_t PortMultiplierPort : 4;
    uint16_t PhyRegionDescTableLength : 16;
    uint32_t PhyRegionDescByteCount;
    uint64_t CommandTableBaseAddress;

    uint32_t Reserved2[4];
} __attribute__((packed)) ahci_command_header_t;

#define AHCI_COMMAND_LIST_COUNT     32
#define AHCI_COMMAND_LIST_SIZE      (sizeof(ahci_command_header_t) * AHCI_COMMAND_LIST_COUNT)

typedef struct {
    uint64_t DataBaseAddress;
    uint32_t Reserved1;

    uint32_t DataByteCount : 22;
    uint16_t Reserved : 9;
    bool InterruptOnCompletion : 1;
} __attribute__((packed)) ahci_prdt_entry_t;

typedef struct {
    uint8_t CommandFis[64];
    uint8_t AtapiCommand[16];
    uint8_t Reserved[48];
    ahci_prdt_entry_t PhysRegionDescTable[1];
} __attribute__((packed)) ahci_command_table_t;

// FIS Register - Host to Device.
typedef struct {
    // DWORD 0.
    uint8_t FisType;
    uint8_t PortMultiplierPort : 4;
    uint8_t Reserved1 : 3;
    bool IsCommand : 1;
    uint8_t CommandReg;
    uint8_t FeaturesLow;

    // DWORD 1.
    uint8_t Lba1[3];
    uint8_t Device;

    // DWORD 2.
    uint8_t Lba2[3];
    uint8_t FeaturesHigh;

    // DWORD 3.
    uint16_t Count;
    uint8_t Icc;
    uint8_t ControlReg;

    // DWORD 4.
    uint32_t Reserved2;
} __attribute__((packed)) ahci_fis_reg_host_to_device_t;

// FIS Register - Device to Host.
typedef struct {
    // DWORD 0.
    uint8_t FisType;
    uint8_t PortMultiplierPort : 4;
    uint8_t Reserved1 : 2;
    bool Interrupt : 1;
    bool Reserved2 : 1;
    uint8_t Status;
    uint8_t Error;

    // DWORD 1.
    uint8_t Lba1[3];
    uint8_t Device;

    // DWORD 2.
    uint8_t Lba2[3];
    uint8_t Reserved3;

    // DWORD 3.
    uint16_t Count;
    uint16_t Reserved4;

    // DWORD 4.
    uint32_t Reserved5;
} __attribute__((packed)) ahci_fis_reg_device_to_host_t;

// FIS PIO Setup – Device to Host
#define AHCI_FIS_TYPE_PIO_SETUP 0x5F
typedef struct {
    // DWORD 0.
    uint8_t FisType; // 0x5F
    uint8_t PortMultiplierPort : 4;
    bool Reserved1 : 1;
    bool IsDeviceToHost : 1;
    bool Interrupt : 1;
    bool Reserved2 : 1;
    uint8_t Status;
    uint8_t Error;

    // DWORD 1.
    uint8_t Lba1[3];
    uint8_t Device;

    // DWORD 2.
    uint8_t Lba2[3];
    uint8_t Reserved3;

    // DWORD 3.
    uint16_t Count;
    uint8_t Reserved4;
    uint8_t NewStatus;

    // DWORD 4.
    uint16_t TransferCount;
    uint16_t Reserved5;
} __attribute__((packed)) ahci_fis_pio_setup_t;

typedef struct {
    uint8_t test[0x20];

    ahci_fis_pio_setup_t PioSetupFis;
    uint8_t PioSetupPad[0x40 - 0x34];

    ahci_fis_reg_device_to_host_t DeviceToHostRegFis;
    uint8_t DeviceToHostRegPad[0x100 - 0x54];
} __attribute__((packed)) ahci_received_fis_t;

struct ahci_controller_t;
typedef struct {
    struct ahci_controller_t *Controller;
    uint8_t Number;
    uint8_t Type;

    ahci_command_header_t *CommandList;
    ahci_received_fis_t *ReceivedFis;
} ahci_port_t;

typedef struct {
    // Register access.
    uint32_t BaseAddress;
    uint32_t *BasePointer;
    ahci_memory_t *Memory;

    // Pages used for memory.
    uintptr_t DmaPage;
    uintptr_t DmaPageSecondary;

    // Ports.
    ahci_port_t **Ports;
    uint8_t PortCount;
} ahci_controller_t;

extern bool ahci_init(pci_device_t *pciDevice);

#endif
