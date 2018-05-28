/*
 * File: ata_commands.h
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

#ifndef ATA_COMMANDS_H
#define ATA_COMMANDS_H

#include <main.h>

#define ATA_DEVICE_RESET            0x08

#define ATA_CMD_READ_SECTOR         0x20
#define ATA_CMD_READ_SECTOR_EXT     0x24
#define ATA_CMD_WRITE_SECTOR        0x30
#define ATA_CMD_PACKET              0xA0
#define ATA_CMD_IDENTIFY_PACKET     0xA1
#define ATA_CMD_IDENTIFY            0xEC

#define ATA_SERIAL_LENGTH           20
#define ATA_FIRMWARE_LENGTH         8
#define ATA_MODEL_LENGTH            40
#define ATA_ADD_ID_LENGTH           8
#define ATA_MEDIA_SERIAL_LENGTH     60

#define ATA_IDENTIFY_INTEGRITY_MAGIC        0xA5
#define ATA_IDENTIFY_GENERAL_NOT_ATA_DEVICE 0x8000
#define ATA_IDENTIFY_GENERAL_ATAPI_DEVICE   0x8000

// Word 80 ATA version flags.
enum {
    ATA_VERSION_ATA1    = 0x02,
    ATA_VERSION_ATA2    = 0x04,
    ATA_VERSION_ATA3    = 0x08,
    ATA_VERSION_ATA4    = 0x10,
    ATA_VERSION_ATA5    = 0x20,
    ATA_VERSION_ATA6    = 0x40,
    ATA_VERSION_ATA7    = 0x80,
    ATA_VERSION_ATA8    = 0x100
};

// Word 76 SATA supported feature flags.
enum {
    ATA_SATA76_GEN1_SUPPORTED       = 0x02,
    ATA_SATA76_GEN2_SUPPORTED       = 0x04,
    ATA_SATA76_NCQ_SUPPORTED        = 0x100,
    ATA_SATA76_POWER_SUPPORTED      = 0x200,
    ATA_SATA76_PHY_EVENTS_SUPPORTED = 0x400
};

// Word 53 of IDENTIFY command.
typedef struct {
    bool Words54_58Valid : 1;
    bool Words64_70Valid : 1;
    bool Word88Valid : 1;
    uint8_t Reserved : 5;

    uint8_t FreeFallControlSensitivity;
} __attribute__((packed)) ata_identify_word53_t;

// Word 59 of IDENTIFY command.
typedef struct {
    uint8_t CurrentSectorsPerInterrupt;
    bool MultipleSectorSettingValid : 1;
    uint8_t Reserved : 7;
} __attribute__((packed)) ata_identify_word59_t;

// Word 63 of IDENTIFY command.
typedef struct {
    bool MultiwordDmaMode0Supported : 1;
    bool MultiwordDmaMode1Supported : 1;
    bool MultiwordDmaMode2Supported : 1;
    uint8_t Reserved1 : 5;

    bool MultiwordDmaMode0Selected : 1;
    bool MultiwordDmaMode1Selected : 1;
    bool MultiwordDmaMode2Selected : 1;
    uint8_t Reserved2 : 5;
} __attribute__((packed)) ata_identify_word63_t;

// Word 69 of IDENTIFY command.
typedef struct {
    uint8_t Reserved : 3;
    bool ExtendedSectorsSupported : 1;
    bool DeviceEncryptsAllData : 1;
    bool TrimmedLbaRangesSupported : 1;
    bool Optional28bitCommandsSupported : 1;
    bool ReservedIeee1667 : 1;

    bool DownloadMicrocodeDmaSupported : 1;
    bool SetMaxSetPasswordDmaSupported : 1;
    bool WriteBufferDmaSupported : 1;
    bool ReadBufferDmaSupported : 1;
    bool DeviceConfigIdentifyDmaSupported: 1;
    bool LongPhysSectorAlignErrorReportingSupported : 1;
    bool DeterministicLbaRangesSupported : 1;
    bool CfastSpecSupported : 1;
} __attribute__((packed)) ata_identify_word69_t;

// Word 76 of IDENTIFY command.
typedef struct {
    bool AlwaysFalse : 1;
    bool Gen1Supported : 1;
    bool Gen2Supported : 1;
    uint8_t Reserved1 : 5;

    bool NcqSupported : 1;
    bool HostInitPowerRequestsSupported : 1;
    bool PhyEventsSupported : 1;
    uint8_t Reserved2 : 5;
} __attribute__((packed)) ata_identify_word76_t;

// Word 78 of IDENTIFY command.
typedef struct {
    bool AlwaysFalse : 1;
    bool NonZeroBufferOffsetsSupported : 1;
    bool DmaSetupAutoActivationSupported : 1;
    bool PowerManagementSupported : 1;
    bool InOrderDataDeliverySupported : 1;
    bool Reserved1 : 1;
    bool SoftwareSettingsPreservationSupported : 1;
    uint16_t Reserved2 : 9;
} __attribute__((packed)) ata_identify_word78_t;

// Word 79 of IDENTIFY command.
typedef struct {
    bool AlwaysFalse : 1;
    bool NonZeroBufferOffsetsEnabled : 1;
    bool DmaSetupAutoActivationEnabled : 1;
    bool PowerManagementEnabled : 1;
    bool InOrderDataDeliveryEnabled : 1;
    bool Reserved1 : 1;
    bool SoftwareSettingsPreservationEnabled : 1;
    uint16_t Reserved2 : 9;
} __attribute__((packed)) ata_identify_word79_t;

// Word 80 of IDENTIFY command.
typedef struct {
    bool Ata1Supported : 1;
    bool Ata2Supported : 1;
    bool Ata3Supported : 1;
    bool Ata4Supported : 1;
    bool Ata5Supported : 1;
    bool Ata6Supported : 1;
    bool Ata7Supported : 1;
    bool Ata8Supported : 1;
    bool Acs2Supported : 1;
    uint8_t Reserved : 6;
} __attribute__((packed)) ata_identify_word80_t;

// Word 82 of IDENTIFY command.
typedef struct {
    bool SmartSupported : 1;
    bool SecuritySupported : 1;
    bool RemovableMediaSupported : 1;
    bool PowerManagementSupported : 1;
    bool PacketSupported : 1;
    bool WriteCacheSupported : 1;
    bool LookAheadSupported : 1;
    bool ReleaseInterruptSupported : 1;
    bool ServiceInterruptSupported : 1;
    bool DeviceResetCommandSupported : 1;
    bool HpaSupported : 1;
    bool Retired1 : 1;
    bool WriteBufferSupported : 1;
    bool ReadBufferSupported : 1;
    bool NopSupported : 1;
    bool Retired2 : 1;
} __attribute__((packed)) ata_identify_word82_t;

// Word 83 of IDENTIFY command.
typedef struct {
    bool DownloadMicrocodeSupported : 1;
    bool ReadWriteDmaQueuedSupported : 1;
    bool CfaSupported : 1;
    bool ApmSupported : 1;
    bool RemoteMediaNotificationSupported : 1;
    bool PowerUpStandbySupported : 1;
    bool SetFeaturesSpinupRequired : 1;
    bool Reserved : 1;
    bool SetMaxExtensionSupported : 1;
    bool AutoAcousticManagementSupported : 1;
    bool Lba48BitSupported : 1;
    bool DeviceConfigOverlaySupported : 1;
    bool MandatoryFlushCacheSupported : 1;
    bool FlushCacheExtSupported : 1;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word83_t;

// Word 85 of IDENTIFY command.
typedef struct {
    bool SmartEnabled : 1;
    bool SecurityEnabled : 1;
    bool RemovableMediaSupported : 1;
    bool PowerManagementSupported : 1;
    bool PacketSupported : 1;
    bool WriteCacheEnabled : 1;
    bool LookAheadEnabled : 1;
    bool ReleaseInterruptEnabled : 1;
    bool ServiceInterruptEnabled : 1;
    bool DeviceResetCommandSupported : 1;
    bool HpaSupported : 1;
    bool Retired1 : 1;
    bool WriteBufferSupported : 1;
    bool ReadBufferSupported : 1;
    bool NopSupported : 1;
    bool Retired2 : 1;
} __attribute__((packed)) ata_identify_word85_t;

// Word 86 of IDENTIFY command.
typedef struct {
    bool DownloadMicrocodeSupported : 1;
    bool ReadWriteDmaQueuedSupported : 1;
    bool CfaSupported : 1;
    bool ApmEnabled : 1;
    bool RemoteMediaNotificationEnabled : 1;
    bool PowerUpStandbyEnabled : 1;
    bool SetFeaturesSpinupRequired : 1;
    bool Reserved1 : 1;
    bool SetMaxExtensionEnabled : 1;
    bool AutoAcousticManagementEnabled : 1;
    bool Lba48BitEnabled : 1;
    bool DeviceConfigOverlayEnabled : 1;
    bool MandatoryFlushCacheEnabled : 1;
    bool FlushCacheExtEnabled : 1;
    bool Reserved2 : 1;
    bool Words119_120Valid : 1;
} __attribute__((packed)) ata_identify_word86_t;

// Word 87 of IDENTIFY command.
typedef struct {
    bool SmartErrorLoggingSupported : 1;
    bool SmartSelfTestSupported : 1;
    bool MediaSerialNumberValid : 1;
    bool MediaCardPassThroughEnabled : 1;
    bool ConfigureStreamExecuted : 1;
    bool GeneralPurposeLoggingSupported : 1;
    bool WriteDmaFuaExtSupported : 1;
    bool WriteDmaQueuedFuaExtSupported : 1;
    bool worldWideName64bitsSupported : 1;
    bool UrgReadStreamDmaExtSupported : 1;
    bool UrgWriteStreamDmaExtSupported : 1;
    bool ReservedTechReport1 : 1;
    bool ReservedTechReport2 : 1;
    bool IdleImmediateSupported : 1;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word87_t;

// Word 88 of IDENTIFY command.
typedef struct {
    bool UltraDmaMode0Supported : 1;
    bool UltraDmaMode1Supported : 1;
    bool UltraDmaMode2Supported : 1;
    bool UltraDmaMode3Supported : 1;
    bool UltraDmaMode4Supported : 1;
    bool UltraDmaMode5Supported : 1;
    bool UltraDmaMode6Supported : 1;
    bool Reserved1 : 1;
    bool UltraDmaMode0Selected : 1;
    bool UltraDmaMode1Selected : 1;
    bool UltraDmaMode2Selected : 1;
    bool UltraDmaMode3Selected : 1;
    bool UltraDmaMode4Selected : 1;
    bool UltraDmaMode5Selected : 1;
    bool UltraDmaMode6Selected : 1;
    bool Reserved2 : 1;
} __attribute__((packed)) ata_identify_word88_t;

// Word 93 of IDENTIFY command.
typedef struct {
    bool AlwaysTrue1 : 1;
    uint8_t Device0Type : 2;
    bool Device0DiagPassed : 1;
    bool Device0PdiagAssert : 1;
    bool Device0DaspAssert : 1;
    bool Device0RespondsForDevice1 : 1;
    bool Device0Reserved : 1;

    bool AlwaysTrue2 : 1;
    uint8_t Device1Type : 2;
    bool Device1PdiagAssert : 1;
    bool Device1Reserved : 1;
    bool HighCblid : 1;
    bool AlwaysTrue3 : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word93_t;

// Word 106 of IDENTIFY command.
typedef struct {
    uint8_t LogicalSectorsPerPhysical : 4;
    uint8_t Reserved : 8;
    bool DeviceLogicalSectorLongerThan256Words : 1;
    bool MultipleLogicalSectorsPerPhysical : 1;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word106_t;

// Word 119 of IDENTIFY command.
typedef struct {
    bool ReservedDdt : 1;
    bool WriteReadVerifySupported : 1;
    bool WriteUncorrectableExtSupported : 1;
    bool ReadWriteLogDmaExtSupported : 1;
    bool DownloadMicrocode3Supported : 1;
    bool FreeFallSupported : 1;
    bool SenseDataReportingSupported : 1;
    bool ExtendedPowerConditionsSupported : 1;
    uint8_t Reserved : 6;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word119_t;

// Word 120 of IDENTIFY command.
typedef struct {
    bool ReservedDdt : 1;
    bool WriteReadVerifyEnabled : 1;
    bool WriteUncorrectableExtSupported : 1;
    bool ReadWriteLogDmaExtSupported : 1;
    bool DownloadMicrocode3Supported : 1;
    bool FreeFallEnabled : 1;
    bool SenseDataReportingEnabled : 1;
    bool ExtendedPowerConditionsEnabled : 1;
    uint8_t Reserved : 6;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word120_t;

// Word 128 of IDENTIFY command.
typedef struct {
    bool SecuritySupported : 1;
    bool SecurityEnabled : 1;
    bool SecurityLocked : 1;
    bool SecurityFrozen : 1;
    bool SecurityCountExpired : 1;
    bool EnhancedSecurityEraseSupported : 1;
    uint8_t Reserved1 : 2;
    bool MasterPasswordMax : 1;
    uint8_t Reserved2 : 7;
} __attribute__((packed)) ata_identify_word128_t;

// Word 160 of IDENTIFY command.
typedef struct {
    uint16_t MaxCurrentMa : 12;
    bool CfaPowerMode1Disabled : 1;
    bool cfaPowerMode1Required : 1;
    bool Reserved : 1;
    bool Word160Supported : 1;
} __attribute__((packed)) ata_identify_word160_t;

// Word 206 of IDENTIFY command.
typedef struct {
    bool SctSupported : 1;
    bool Retired : 1;
    bool SctWriteSameSupported : 1;
    bool SctErrorRecoveryControlSupported : 1;
    bool SctFeatureControlSupported : 1;
    bool SctDataTablesSupported : 1;
    uint8_t Reserved : 6;
    uint8_t VendorSpecific : 4;
} __attribute__((packed)) ata_identify_word206_t;

// Word 209 of IDENTIFY command.
typedef struct {
    uint16_t LogicalSectorOffset : 14;
    bool AlwaysTrue : 1;
    bool AlwaysFalse : 1;
} __attribute__((packed)) ata_identify_word209_t;

// Word 214 of IDENTIFY command.
typedef struct {
    bool NvCachePowerModeSupported : 1;
    bool NvCachePowerModeEnabled : 1;
    uint8_t Reserved1 : 2;
    bool NvCacheEnabled : 1;
    uint8_t Reserved2 : 3;
    uint8_t NvCachePowerModeVersion : 4;
    uint8_t NvCacheVersion : 4;
} __attribute__((packed)) ata_identify_word214_t;

// Result of IDENTIFY command.
typedef struct {
    // General configuration bits. Word 0.
    uint16_t GeneralConfig;

    // Number of logical cylinders (ATA-1 to ATA-5). Word 1.
    uint16_t LogicalCylinders;

    // Specific configuration (ATA-5+). Word 2.
    uint16_t SpecificConfig;

    // Number of logical heads (ATA-1 to ATA-5). Word 3.
    uint16_t LogicalHeads;

    // Vendor specific words 4 and 5.
    uint16_t VendorSpecific4[2];

    // Number of logical sectors per track (ATA-1 to ATA-5). Word 6.
    uint16_t LogicalSectorsPerTrack;

    // Vendor specific words 7, 8, and 9.
    uint16_t VendorSpecific7[3];

    // Serial number (20 ASCII characters). Words 10-19.
    char Serial[ATA_SERIAL_LENGTH];

    // Vendor specific words 20, 21, and 22.
    uint16_t VendorSpecific20[2];
    uint16_t VendorReadWriteLongBytes;

    // Firmware revision (8 ASCII characters). Words 23-26.
    char FirmwareRevision[ATA_FIRMWARE_LENGTH];

    // Model name (40 ASCII characters). Words 27-46.
    char Model[ATA_MODEL_LENGTH];

    // Maximum number of sectors to be transferred in multi-sector read/write commands. Word 47.
    uint8_t MaxSectorsInterrupt;
    uint8_t Reserved47;

    // Trusted computing info (ATA-8+). Word 48.
    uint16_t TrustedComputingFlags;

    // Capabilities. Word 49.
    uint16_t Capabilities49;

    // Capabilities (ATA-4+). Word 50.
    uint16_t Capabilities50;

    // PIO transfer mode. (ATA-1 to ATA-4). Words 51 and 52.
    uint8_t VendorSpecific51;
    uint8_t PioMode;
    uint8_t VendorSpecific52;
    uint8_t Retired52;

    // Flags indicating valid IDENTIFY word ranges (ATA-1+), and free-fall sensitivity (ATA-8+). Word 53.
    ata_identify_word53_t Flags53;

    // Number of current logical cylinders, heads, and sectors per track. Obsolete after ATA-5. Words 54-56.
    uint16_t CurrentLogicalCylinders;
    uint16_t CurrentLogicalHeads;
    uint16_t CurrentLogicalSectorsPerTrack;

    // Current capacity in sectors. Obsolete after ATA-5. Words 57-58.
    uint32_t CurrentCapacitySectors;

    // Flags indicating current number of sectors to be transferred in multi-read/write commands. Word 59.
    ata_identify_word59_t Flags59;

    // Total number of LBA sectors for 28-bit commands. Only valid if LBA is supported. Words 60-61.
    uint32_t TotalLbaSectors28Bit;

    // Retired word 62.
    uint16_t Retired62;

    // Flags indicating multiword DMA status. Word 63.
    ata_identify_word63_t MultiwordDmaFlags;

    // Flags indicuating supported PIO modes (ATA-2+). Word 64.
    uint8_t Reserved64;
    uint8_t PioModesSupported;

    // Cycles times for multiword DMA and PIO (ATA-2+). Words 65-68.
    uint16_t MultiwordDmaMinCycleTime;
    uint16_t MultiwordDmaRecCycleTime;
    uint16_t PioMinCycleTimeNoFlow;
    uint16_t PioMinCycleTimeIoRdy;

    // Additional supported flags (ATA-8+). Words 69 and 70.
    ata_identify_word69_t AdditionalSupportedFlags;
    uint16_t Reserved70;

    // Reserved words 71 to 74.
    uint16_t ReservedIdentifyPacket[4];

    // Maximum queue depth - 1 (ATA-4+). Lower 5 bits of word 75.
    uint8_t MaxQueueDepth : 5;
    uint16_t Reserved75 : 11;

    // Serial ATA flags (ATA-8+). Words 76 to 79.
    ata_identify_word76_t SerialAtaCapabilites76;
    uint16_t SerialAtaReserved77;
    ata_identify_word78_t SerialAtaFeatures78;
    ata_identify_word79_t SerialAtaFeatures79;

    // Version flags (ATA-3+). Words 80 and 81.
    ata_identify_word80_t VersionMajor;
    uint16_t VersionMinor;

    // Supported command flags (ATA-3+). Words 82 to 87.
    ata_identify_word82_t CommandSets82;
    ata_identify_word83_t CommandSets83;
    uint16_t CommandSetsReserved84;
    ata_identify_word85_t CommandSets85;
    ata_identify_word86_t CommandSets86;
    ata_identify_word87_t CommandSets87;

    // Ultra DMA mode flags (ATA-4+) Word 88.
    ata_identify_word88_t UltraDmaMode;

    // Times required for SECURITY ERASE UNIT command (ATA-4+). Lower halves of words 89 and 90.
    uint8_t Reserved89;
    uint8_t NormalEraseTime;
    uint8_t Reserved90;
    uint8_t EnhancedEraseTime;

    // Current APM level starting (ATA-4+). Word 91.
    uint16_t CurrentApmLevel;

    // Master password revision code (ATA-5+). Word 92.
    uint16_t MasterPasswordRevision;

    // Hardware reset result (ATA-5+). Word 93.
    ata_identify_word93_t HardwareResetResult;

    // Acoustic info (ATA-6 to ATA-8). High and low halfs of word 94.
    uint8_t RecommendedAcousticValue;
    uint8_t CurrentAcousticValue;

    // Stream info (ATA-7+). Words 95-99.
    uint16_t StreamMinSize;
    uint16_t StreamTransferTime;
    uint16_t StreamAccessLatency;
    uint32_t StreamPerfGranularity; // High in word 98, low in word 99.

    // Number of LBA sectors for 48-bit LBA (ATA-6+). Words 100-103.
    uint64_t TotalLba48Bit;

    // Stream tranfer time for PIO (ATA-7+). Word 104.
    uint16_t StreamTransferTimePio;

    // Word 105.
    uint16_t Max512ByteBlocksDataSetManagment;

    // Physical sector size flags (ATA-7+). Word 106.
    ata_identify_word106_t PhysicalSectorSize;

    // Inter-seek delay (ATA-7+). Word 107.
    uint16_t InterSeekDelay;

    // World wide name (ATA-7+). Words 108-111.
    uint64_t WorldWideName;

    // Reserved words 112 to 116.
    uint16_t Reserved112[5];

    // Logical sector size (ATA-7+). Words 117 and 118.
    uint32_t LogicalSectorSize;

    // Supported command flags (ATA-8+). Words 119 and 120.
    ata_identify_word119_t CommandSets119;
    ata_identify_word120_t CommandSets120;

    // Reserved words 121 to 126.
    uint16_t Reserved121[6];

    // Removable media flags (ATA-4 to ATA-7). Word 127.
    uint16_t RemovableMediaFlags;

    // Security status flags (ATA-3+). Word 128.
    ata_identify_word128_t SecurityFlags;

    // Vendor specific words 129 to 159.
    uint16_t VendorSpecific129[31];

    // CFA power mode flags (ATA-5+). Word 160.
    ata_identify_word160_t CfaFlags;

    // Compact Flash reserved words 161 to 167.
    uint16_t Reserved161[7];

    // Device form factor (ATA-8+). Lower 4 bits of word 168.
    uint8_t FormFactor : 4;
    uint16_t Reserved168 : 12;

    // Data set management flags (TRIM) (ATA-8+). Word 169.
    bool DataSetTrimSupported : 1;
    uint16_t Reserved : 15;

    // Additional product identifier (words 170-173).
    char AdditionalIdentifier[ATA_ADD_ID_LENGTH];

    // Reserved words 174 and 175.
    uint16_t Reserved174[2];

    // Current media serial number string (ATA-6+) Words 176 to 205.
    char MediaSerial[ATA_MEDIA_SERIAL_LENGTH];

    // SCT command transport flags (ATA-8+). Word 206.
    ata_identify_word206_t SctCommandTransportFlags;

    // Reserved words 207 and 208.
    uint16_t Reserved207[2];

    // Alignment of logical blocks within a physical block (ATA-8+). Word 209.
    ata_identify_word209_t PhysicalBlockAlignment;

    // Write-Read-Verify Sector Counts (ATA-8+). Words 210-211, and 212-213.
    uint32_t WriteReadVerifySectorCountMode3;
    uint32_t WriteReadVerifySectorCountMode2;

    // NV cache capabilities (ATA-8+). Word 214.
    ata_identify_word214_t NvCacheCapabilities;

    // NV cache size in blocks (ATA-8+). Words 215 and 216.
    uint32_t NvCacheSize;

    // Media rotation rate (ATA-8+). Word 217.
    uint16_t RotationRate;

    // Reserved word 218.
    uint16_t Reserved218;

    // NV cache options (ATA-8+). Word 219.
    uint8_t EstimatedSpinUpTime;
    uint8_t Reserved219;

    // Write-Read-Verify feature set current mode (ATA-8+). Lower half of word 220.
    uint8_t RriteReadVerifyCurrentMode;
    uint8_t Reserved220;

    // Reserved word 221.
    uint16_t Reserved221;

    // Transport version numbers (ATA-8+). Words 222 and 223.
    uint16_t transportVersionMajor;
    uint16_t transportVersionMinor;

    // Reserved words 224 to 229.
    uint16_t Reserved224[6];

    // Extended number of LBA sectors (words 230-233).
    uint64_t ExtendedSectors;

    // Min and max 512 byte blocks for Download Microcode mode 3. Words 234 and 235.
    uint16_t Min512ByteBlocksPerDownloadMicrocode3;
    uint16_t Max512ByteBlocksPerDownloadMicrocode3;

    // Reserved words 236 to 254.
    uint16_t Reserved236[19];

    // Integrity word 255.
    uint8_t ChecksumValidityIndicator;
    uint8_t Checksum;
} __attribute__((packed)) ata_identify_result_t;

struct ata_identify_packet_general_config {
    uint8_t packetSize : 2;
    uint8_t reserved1 : 3;
    uint8_t drqResponseType : 2;
    bool removableDevice : 1;
    uint8_t deviceType : 5;
    uint8_t reserved2 : 1;
    uint8_t atapiType : 2;
};
typedef struct ata_identify_packet_general_config ata_identify_packet_general_config_t;

// Result of IDENTIFY PACKET DEVICE command.
struct ata_identify_packet_result {
    // General configuration bits. Word 0.
    union {
        uint16_t data;
        ata_identify_packet_general_config_t info;
    } generalConfig;

    // Specific configuration (ATAPI-5+). Word 7.
    uint16_t specificConfig;

    // Serial number (20 ASCII characters plus null). Words 10-19.
    char serial[ATA_SERIAL_LENGTH+1];

    // Firmware revision (8 ASCII characters plus null). Words 23-26.
    char firmwareRevision[ATA_FIRMWARE_LENGTH+1];

    // Model name (40 ASCII characters plus null). Words 27-46.
    char model[ATA_MODEL_LENGTH+1];

    // Capabilities. Word 49.
    uint16_t capabilities49;

    // Capabilities (ATA-4+). Word 50.
    uint16_t capabilities50;

    // PIO transfer mode. (ATAPI-4). High half of word 51.
    uint8_t pioMode;

    // Flags indicating valid IDENTIFY word ranges. Word 53.
    uint16_t flags53;

    // DMA flags (ATAPI-7+). Word 62.
    uint16_t dmaFlags62;

    // Flags indicating multiword DMA status. Word 63.
    uint16_t multiwordDmaFlags;

    // Flags indicuating supported PIO modes. Lower half of word 64.
    uint8_t pioModesSupported;

    // Cycles times for multiword DMA and PIO. Words 65-68.
    uint16_t multiwordDmaMinCycleTime;
    uint16_t multiwordDmaRecCycleTime;
    uint16_t pioMinCycleTimeNoFlow;
    uint16_t pioMinCycleTimeIoRdy;

    // Time for PACKET and SERVICE commands (ATAPI-4 to ATAPI-7). Words 71 and 72.
    uint16_t timePacketRelease;
    uint16_t timeServiceBusy;

    // Maximum queue depth - 1 (ATAPI-4 to ATAPI-7). Lower 5 bits of Word 75.
    uint8_t maxQueueDepth;

    // Serial ATA flags (ATAPI-8+). Words 76, 78, and 79.
    uint16_t serialAtaFlags76;
    uint16_t serialAtaFlags78;
    uint16_t serialAtaFlags79;

    // Version flags. Words 80 and 81.
    uint16_t versionMajor;
    uint16_t versionMinor;

    // Supported command flags. Words 82-87.
    uint16_t commandFlags82;
    uint16_t commandFlags83;
    uint16_t commandFlags84;
    uint16_t commandFlags85;
    uint16_t commandFlags86;
    uint16_t commandFlags87;

    // Ultra DMA mode flags. Word 88.
    uint16_t ultraDmaMode;

    // Times required for SECURITY ERASE UNIT command (ATAPI-8+). Lower halves of words 89 and 90.
    uint8_t normalEraseTime;
    uint8_t enhancedEraseTime;

    // Current APM level (ATAPI-8+). Word 91.
    uint16_t currentApmLevel;

    // Master password revision code (ATAPI-8+). Word 92.
    uint16_t masterPasswordRevision;

    // Hardware reset result (ATAPI-5+). Word 93.
    uint16_t hardwareResetResult;

    // Acoustic info (ATAPI-6 to ATAPI-8). High and low halfs of word 94.
    uint8_t recommendedAcousticValue;
    uint8_t currentAcousticValue;

    // World wide name (ATAPI-7+). Words 108-111.
    uint64_t worldWideName;

    // Supported command flags (ATAPI-8+). Words 119 and 120.
    uint16_t commandFlags119;
    uint16_t commandFlags120;

    // Removable media flags (ATAPI-4 to ATAPI-7). Word 127.
    uint16_t removableMediaFlags;

    // Security status flags. Word 128.
    uint16_t securityFlags;

    // Transport version numbers (ATA-8+). Words 222 and 223.
    uint16_t transportVersionMajor;
    uint16_t transportVersionMinor;
};
typedef struct ata_identify_packet_result ata_identify_packet_result_t;

extern uint16_t ata_read_identify_word(ata_channel_t *channel, uint8_t *checksum);
extern void ata_read_identify_words(ata_channel_t *channel, uint8_t *checksum, uint8_t firstWord, uint8_t lastWord);

extern int16_t ata_identify(ata_channel_t *channel, bool master, ata_identify_result_t *outResult);
extern int16_t ata_read_sector(ata_device_t *ataDevice, uint64_t startSectorLba, void *outData, uint32_t length);

#endif
