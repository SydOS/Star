/*
 * File: ahci_port.h
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

#ifndef AHCI_PORT_H
#define AHCI_PORT_H

#include <main.h>
#include <driver/pci.h>
#include <driver/storage/ata/ata.h>

#define AHCI_DEV_TYPE_NONE          0
#define AHCI_DEV_TYPE_SATA          1
#define AHCI_DEV_TYPE_SEMB          2
#define AHCI_DEV_TYPE_PORT_MULT     3
#define AHCI_DEV_TYPE_SATA_ATAPI    4

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
    union {
        uint8_t RawValue;
        ata_reg_status_t Data;
    } Status;
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

#define AHCI_SATA_STATUS_DETECT_INIT_NO_ACTION      0x0
#define AHCI_SATA_STATUS_DETECT_INIT_RESET          0x1
#define AHCI_SATA_STATUS_DETECT_INIT_DISABLE        0x4

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

    union {
        uint32_t RawValue;
        ahci_port_interrupt_t Data;
    } InterruptsStatus;
    union {
        uint32_t RawValue;
        ahci_port_interrupt_t Data;
    } InterruptsEnabled;

    ahci_port_command_status_t CommandStatus;
    uint8_t Reserved1[0x20-0x1C];

    ahci_port_task_file_data_t TaskFileData;

    union {
        uint32_t Value;
        ahci_port_signature_t Data;
    } Signature;

    union {
        uint32_t RawValue;
        ahci_port_sata_status_t Data;
    } SataStatus;

    ahci_port_sata_control_t SataControl;
    union {
        uint32_t RawValue;
        ahci_port_sata_error_t Data;
    } SataError;
    
    uint32_t SataActive;

    uint32_t CommandsIssued;
    ahci_port_sata_notification_t SataNotification;
    ahci_port_fis_switching_control_t FisSwitchingControl;
    ahci_port_device_sleep_t DeviceSleep;

    uint8_t Reserved2[0x70-0x48];
    uint8_t VendorReserved[0x80-0x70];
} __attribute__((packed)) ahci_port_memory_t;

#endif
