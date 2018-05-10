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

#define AHCI_DEV_TYPE_NONE          0
#define AHCI_DEV_TYPE_SATA          1
#define AHCI_DEV_TYPE_SEMB          2
#define AHCI_DEV_TYPE_PORT_MULT     3
#define AHCI_DEV_TYPE_SATA_ATAPI    4

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

// Offset 0x10 and 0x14, Port x Interrupt Status.
typedef struct {
    bool DeviceToHostRegisterFis : 1;
    bool PioSetupFis : 1;
    bool DmaSetupFis : 1;
    bool SetDeviceBits : 1;
    bool UnknownFis : 1;

    bool DescriptorProcessed : 1;
    bool PortChanged : 1;
    bool DeviceMechanicalPresenceChanged : 1;
    uint32_t Reserved1 : 14;
    
    bool PhyRdyChanged : 1;
    bool IncorrectPortMultiplierStatus : 1;
    bool OverflowStatus : 1;
    bool Reserved2 : 1;
    
    bool InterfaceNonFatalError : 1;
    bool InterfaceFatalError : 1;
    bool HostBusDataError : 1;
    bool HostBusFatalError : 1;
    bool TaskFileError : 1;
    bool ColdPortDetected : 1;
} __attribute__((packed)) ahci_port_interrupt_t;

// Offset 0x18, Port x Command and Status.
typedef struct {
    bool Started : 1;
    bool SpinupDevice : 1;
    bool PowerOnDevice : 1;
    bool CommandListOverride : 1;
    bool FisReceiveEnabled : 1;
    uint8_t Reserved : 3;

    uint8_t CurrentCommandSlot : 5;
    bool MechanicalPresenceSwitchOpen : 1;
    bool FisReceiveRunning : 1;
    bool CommandListRunning : 1;
    bool ColdPresenceDeviceAttached : 1;
    bool PortMultiplierAttached : 1;
    bool HotplugCapable : 1;
    bool MechanicalPresenceSwitchPresent : 1;
    bool ColdPresencePresent : 1;

    bool ExternalPort : 1;
    bool FisSwitchingCapable : 1;
    bool AutoPartialToSlumberEnabled : 1;
    bool AtapiDevice : 1;
    bool DriveLedAtapiEnabled : 1;

    bool AggressiveLinkPowerManagementEnabled : 1;
    bool AggressiveSlumberEnabled : 1;
    uint8_t InterfaceCommunicationsControl : 4;
} __attribute__((packed)) ahci_port_command_status_t;

// Offset 0x20, Port x Task File Data.
typedef struct {
    uint8_t Status;
    uint8_t Error;
    uint16_t Reserved;
} __attribute__((packed)) ahci_port_task_file_data_t;

#define AHCI_SIG_ATA        0x00000101
#define AHCI_SIG_ATAPI      0xEB140101
#define AHCI_SIG_SEMB       0xC33C0101
#define AHCI_SIG_PORT_MULT  0x96690101

// Offset 0x24, Port x Signature.
typedef struct {
    uint8_t SectorCount;
    uint8_t LbaLow;
    uint8_t LbaMid;
    uint8_t LbaHigh;
} __attribute__((packed)) ahci_port_signature_t;

#define AHCI_SATA_STATUS_IPM_NOT_PRESENT    0x0
#define AHCI_SATA_STATUS_IPM_ACTIVE         0x1
#define AHCI_SATA_STATUS_IPM_PARTIAL        0x2
#define AHCI_SATA_STATUS_IPM_SLUMBER        0x6
#define AHCI_SATA_STATUS_IPM_DEVSLEEP       0x8

#define AHCI_SATA_STATUS_SPEED_NOT_PRESENT  0x0
#define AHCI_SATA_STATUS_SPEED_GEN1         0x1
#define AHCI_SATA_STATUS_SPEED_GEN2         0x2
#define AHCI_SATA_STATUS_SPEED_GEN3         0x3

#define AHCI_SATA_STATUS_DETECT_NOT_PRESENT 0x0
#define AHCI_SATA_STATUS_DETECT_NO_PHY      0x1
#define AHCI_SATA_STATUS_DETECT_CONNECTED   0x3
#define AHCI_SATA_STATUS_DETECT_OFFLINE     0x4

// Offset 0x28, Port x Serial ATA Status.
typedef struct {
    uint8_t DeviceDetection : 4;
    uint8_t CurrentInterfaceSpeed : 4;
    uint8_t InterfacePowerManagement: 4;
    uint32_t Reserved : 20;
} __attribute__((packed)) ahci_port_sata_status_t;

// Offset 0x2C, Port x Serial ATA Control.
typedef struct {
    uint8_t DeviceDetectionInitialization : 4;
    uint8_t SpeedAllowed : 4;
    uint8_t InterfacePowerTransitionsAllowed : 4;

    uint8_t SelectPowerManagement : 4;
    uint8_t PortMultiplierPort : 4;
    uint16_t Reserved : 12;
} __attribute__((packed)) ahci_port_sata_control_t;

// Offset 0x30, Port x Serial ATA Error.
typedef struct {
    bool RecoveredDataIntegrityError : 1;
    bool RecoveredCommError : 1;
    uint8_t Reserved1 : 6;

    bool TransientDataIntegrityError : 1;
    bool PersistentCommError : 1;
    bool ProtocolError : 1;
    bool InternalError : 1;
    uint8_t Reserved2 : 4;

    bool PhyRdyChanged : 1;
    bool PhyInternalError : 1;
    bool CommWake : 1;
    bool DecodeError : 1;
    bool DisparityError : 1;
    bool CrcError : 1;
    bool HandshakeError : 1;
    bool LinkSequenceError : 1;
    bool TransportTransitionError : 1;
    bool UnknownFisType : 1;
    bool Exchanged : 1;
    uint8_t Reserved3 : 5;
} __attribute__((packed)) ahci_port_sata_error_t;

// Offset 0x3C, Port x Serial ATA Notification.
typedef struct {
    uint16_t PmNotify;
    uint16_t Reserved;
} __attribute__((packed)) ahci_port_sata_notification_t;

// Offset 0x40, Port x FIS-based Switching Control.
typedef struct {
    bool Enabled : 1;
    bool DeviceErrorClear : 1;
    bool SingleDeviceError : 1;
    uint8_t Reserved1 : 5;

    uint8_t DeviceToIssue : 4;
    uint8_t ActiveDeviceOptimization : 4;
    uint8_t DeviceWithError : 4;
    uint16_t Reserved2 : 12;
} __attribute__((packed)) ahci_port_fis_switching_control_t;

typedef struct {
    bool AggressiveDeviceSleepEnabled : 1;
    bool DeviceSleepPresent : 1;
    uint8_t DeviceSleepExitTimeout : 8;
    uint8_t MinDeviceSleepAssertTime : 5;
    uint16_t DeviceSleepIdleTimeout : 10;
    uint8_t DitoMultiplier : 4;
    uint8_t Reserved : 3;
} __attribute__((packed)) ahci_port_device_sleep_t;

typedef struct {
    uint64_t CommandListBaseAddress;
    uint64_t FisBaseAddress;

    ahci_port_interrupt_t InterruptsStatus;
    ahci_port_interrupt_t InterruptsEnabled;

    ahci_port_command_status_t CommandStatus;
    uint8_t Reserved1[0x20-0x1C];

    ahci_port_task_file_data_t TaskFileData;

    union {
        uint32_t Value;
        ahci_port_signature_t Data;
    } Signature;

    ahci_port_sata_status_t SataStatus;
    ahci_port_sata_control_t SataControl;
    ahci_port_sata_error_t SataError;
    uint32_t SataActive;

    uint32_t CommandsIssued;
    ahci_port_sata_notification_t SataNotification;
    ahci_port_fis_switching_control_t FisSwitchingControl;
    ahci_port_device_sleep_t DeviceSleep;

    uint8_t Reserved2[0x70-0x48];
    uint8_t VendorReserved[0x80-0x70];
} __attribute__((packed)) ahci_port_memory_t;

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

struct ahci_controller_t;
typedef struct {
    struct ahci_controller_t *Controller;
    uint8_t Number;

    uint8_t Type;
} ahci_port_t;

typedef struct {
    // Register access.
    uint32_t BaseAddress;
    uint32_t *BasePointer;
    ahci_memory_t *Memory;

    // Ports.
    ahci_port_t **Ports;
    uint8_t PortCount;
} ahci_controller_t;

extern bool ahci_init(pci_device_t *pciDevice);

#endif
