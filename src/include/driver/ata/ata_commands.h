#ifndef ATA_COMMANDS_H
#define ATA_COMMANDS_H

#include <main.h>

#define ATA_CMD_IDENTIFY        0xEC

#define ATA_SERIAL_LENGTH       20
#define ATA_FIRMWARE_LENGTH     8
#define ATA_MODEL_LENGTH        40
#define ATA_ADD_ID_LENGTH       8
#define ATA_MEDIA_SERIAL_LENGTH 60

#define ATA_IDENTIFY_INTEGRITY_MAGIC 0xA5

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

    // Additional product identifier.
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

    // Extended number of LBA sectors.
    uint64_t extendedSectors;
};
typedef struct ata_identify_result ata_identify_result_t;

extern bool ata_identify(uint16_t portCommand, uint16_t portControl, bool master, ata_identify_result_t *outResult);

#endif
