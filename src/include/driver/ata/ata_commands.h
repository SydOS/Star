#ifndef ATA_COMMANDS_H
#define ATA_COMMANDS_H

#include <main.h>

#define ATA_DEVICE_RESET        0x08

#define ATA_CMD_READ_SECTOR     0x20
#define ATA_CMD_WRITE_SECTOR    0x30
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

#define ATA_SERIAL_LENGTH       20
#define ATA_FIRMWARE_LENGTH     8
#define ATA_MODEL_LENGTH        40
#define ATA_ADD_ID_LENGTH       8
#define ATA_MEDIA_SERIAL_LENGTH 60

#define ATA_IDENTIFY_INTEGRITY_MAGIC 0xA5
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

// Result of IDENTIFY command.
struct ata_identify_result {
    // General configuration bits. Word 0.
    uint16_t generalConfig;

    // Number of logical cylinders and heads (ATA-1 to ATA-5). Words 1 and 3.
    uint16_t logicalCylinders;
    uint16_t logicalHeads;

    // Number of logical sectors per track (ATA-1 to ATA-5). Word 6.
    uint16_t logicalSectorsPerTrack;

    // Specific configuration (ATA-5+). Word 7.
    uint16_t specificConfig;

    // Serial number (20 ASCII characters plus null). Words 10-19.
    char serial[ATA_SERIAL_LENGTH+1];

    // Firmware revision (8 ASCII characters plus null). Words 23-26.
    char firmwareRevision[ATA_FIRMWARE_LENGTH+1];

    // Model name (40 ASCII characters plus null). Words 27-46.
    char model[ATA_MODEL_LENGTH+1];

    // Maximum number of sectors to be transferred in multi-sector read/write commands. Low half of word 47
    uint8_t maxSectorsInterrupt;

    // Trusted computing info (ATA-8+). Word 48.
    uint16_t trustedComputingFlags;

    // Capabilities. Word 49.
    uint16_t capabilities49;

    // Capabilities (ATA-4+). Word 50.
    uint16_t capabilities50;

    // PIO transfer mode. (ATA-1 to ATA-4). High half of word 51.
    uint8_t pioMode;

    // Flags indicating valid IDENTIFY word ranges (ATA-1+), and free-fall sensitivity (ATA-8+). Word 53.
    uint16_t flags53;

    // Number of current logical cylinders, heads, and sectors per track. Obsolete after ATA-5. Words 54-56.
    uint16_t currentLogicalCylinders;
    uint16_t currentLogicalHeads;
    uint16_t currentLogicalSectorsPerTrack;

    // Current capacity in sectors. Obsolete after ATA-5. Words 57-58.
    uint32_t currentCapacitySectors;

    // Flags indicating current number of sectors to be transferred in multi-read/write commands.
    uint16_t flags59;

    // Total number of LBA sectors for 28-bit commands. Only valid if LBA is supported. Words 60-61.
    uint32_t totalLba28Bit;

    // Flags indicating multiword DMA status. Word 63.
    uint16_t multiwordDmaFlags;

    // Flags indicuating supported PIO modes (ATA-2+). Lower half of word 64.
    uint8_t pioModesSupported;

    // Cycles times for multiword DMA and PIO (ATA-2+). Words 65-68.
    uint16_t multiwordDmaMinCycleTime;
    uint16_t multiwordDmaRecCycleTime;
    uint16_t pioMinCycleTimeNoFlow;
    uint16_t pioMinCycleTimeIoRdy;

    // Additional supported flags (ATA-8+). Word 69.
    uint16_t additionalSupportedFlags;

    // Maximum queue depth - 1 (ATA-4+). Lower 5 bits of Word 75.
    uint8_t maxQueueDepth;

    // Serial ATA flags (ATA-8+). Words 76, 78, and 79.
    uint16_t serialAtaFlags76;
    uint16_t serialAtaFlags78;
    uint16_t serialAtaFlags79;

    // Version flags (ATA-3+). Words 80 and 81.
    uint16_t versionMajor;
    uint16_t versionMinor;

    // Supported command flags (ATA-3+). Words 82 and 83.
    uint16_t commandFlags82;
    uint16_t commandFlags83;

    // Supported command flags (ATA-4+). Words 84-87.
    uint16_t commandFlags84;
    uint16_t commandFlags85;
    uint16_t commandFlags86;
    uint16_t commandFlags87;

    // Ultra DMA mode flags (ATA-4+) Word 88.
    uint16_t ultraDmaMode;

    // Times required for SECURITY ERASE UNIT command (ATA-4+). Lower halves of words 89 and 90.
    uint8_t normalEraseTime;
    uint8_t enhancedEraseTime;

    // Current APM level starting (ATA-4+). Word 91.
    uint16_t currentApmLevel;

    // Master password revision code (ATA-5+). Word 92.
    uint16_t masterPasswordRevision;

    // Hardware reset result (ATA-5+). Word 93.
    uint16_t hardwareResetResult;

    // Acoustic info (ATA-6 to ATA-8). High and low halfs of word 94.
    uint8_t recommendedAcousticValue;
    uint8_t currentAcousticValue;

    // Stream info (ATA-7+). Words 95-99.
    uint16_t streamMinSize;
    uint16_t streamTransferTime;
    uint16_t streamAccessLatency;
    uint32_t streamPerfGranularity; // High in word 98, low in word 99.

    // Number of LBA sectors for 48-bit LBA (ATA-6+). Words 100-103.
    uint64_t totalLba48Bit;

    // Stream tranfer time for PIO (ATA-7+). Word 104.
    uint16_t streamTransferTimePio;

    // Physical sector size flags (ATA-7+). Word 106.
    uint16_t physicalSectorSize;

    // Inter-seek delay (ATA-7+). Word 107.
    uint16_t interSeekDelay;

    // World wide name (ATA-7+). Words 108-111.
    uint64_t worldWideName;

    // Logical sector size (ATA-7+). Words 117 and 118.
    uint32_t logicalSectorSize;

    // Supported command flags (ATA-8+). Words 119 and 120.
    uint16_t commandFlags119;
    uint16_t commandFlags120;

    // Removable media flags (ATA-4 to ATA-7). Word 127.
    uint16_t removableMediaFlags;

    // Security status flags (ATA-3+). Word 128.
    uint16_t securityFlags;

    // CFA power mode flags (ATA-5+). Word 160.
    uint16_t cfaFlags;

    // Device form factor (ATA-8+). Lower 4 bits of word 168.
    uint8_t formFactor;

    // Data set management flags (TRIM) (ATA-8+). Word 169.
    uint16_t dataSetManagementFlags;

    // Additional product identifier (words 170-173).
    char additionalIdentifier[ATA_ADD_ID_LENGTH+1];

    // Current media serial number string (ATA-6+) Words 176 to 205.
    char mediaSerial[ATA_MEDIA_SERIAL_LENGTH+1];

    // SCT command transport flags (ATA-8+). Word 206.
    uint16_t sctCommandTransportFlags;

    // Alignment of logical blocks within a physical block (ATA-8+). Word 209.
    uint16_t physicalBlockAlignment;

    // Write-Read-Verify Sector Counts (ATA-8+). Words 210-211, and 212-213.
    uint32_t writeReadVerifySectorCountMode3;
    uint32_t writeReadVerifySectorCountMode2;

    // NV cache capabilities (ATA-8+). Word 214.
    uint16_t nvCacheCapabilities;

    // NV cache size in blocks (ATA-8+). Words 215 and 216.
    uint32_t nvCacheSize;

    // Media rotation rate (ATA-8+). Word 217.
    uint16_t rotationRate;

    // NV cache options (ATA-8+). Word 219.
    uint16_t nvCacheFlags;

    // Write-Read-Verify feature set current mode (ATA-8+). Lower half of word 220.
    uint8_t writeReadVerifyCurrentMode;

    // Transport version numbers (ATA-8+). Words 222 and 223.
    uint16_t transportVersionMajor;
    uint16_t transportVersionMinor;

    // Extended number of LBA sectors (words 230-233).
    uint64_t extendedSectors;
};
typedef struct ata_identify_result ata_identify_result_t;

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

extern bool ata_identify(uint16_t portCommand, uint16_t portControl, bool master, ata_identify_result_t *outResult);

#endif
