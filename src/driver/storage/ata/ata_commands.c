/*
 * File: ata_commands.c
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

#include <main.h>
#include <io.h>
#include <driver/storage/ata/ata.h>
#include <driver/storage/ata/ata_commands.h>


uint16_t ata_read_identify_word(ata_channel_t *channel, uint8_t *checksum) {
    // Read word, adding value to checksum.
    uint16_t data = ata_read_data_word(channel->CommandPort);
    *checksum += (uint8_t)(data >> 8);
    *checksum += (uint8_t)(data & 0xFF);
    return data;
}

void ata_read_identify_words(ata_channel_t *channel, uint8_t *checksum, uint8_t firstWord, uint8_t lastWord) {
    for (uint8_t word = firstWord; word <= lastWord; word++)
        ata_read_identify_word(channel, checksum);
}

int16_t ata_identify(ata_channel_t *channel, bool master, ata_identify_result_t *outResult) {
    // Send IDENTIFY command.
    ata_send_command(channel, 0x00, 0x00, 0x00, 0x00, ATA_CMD_IDENTIFY);

    // Wait for device.
    if (ata_wait_for_irq(channel, master) != ATA_CHK_STATUS_OK || !ata_wait_for_drq(channel))
       return ata_check_status(channel, master);

    // Checksum total, used at end.
    uint8_t checksum = 0;
    ata_identify_result_t result = {};

    // Read words 0-9.
    result.generalConfig = ata_read_identify_word(channel, &checksum);
    result.logicalCylinders = ata_read_identify_word(channel, &checksum);
    result.specificConfig = ata_read_identify_word(channel, &checksum);
    result.logicalHeads = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 4, 5);
    result.logicalSectorsPerTrack = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 7, 9);

    // Read serial number (words 10-19).
    for (uint16_t i = 0; i < ATA_SERIAL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(channel, &checksum);
        result.serial[i] = (char)(value >> 8);
        result.serial[i+1] = (char)(value & 0xFF);
    }
    result.serial[ATA_SERIAL_LENGTH] = '\0';

    // Unused words 20-22.
    ata_read_identify_words(channel, &checksum, 20, 22);

    // Read firmware revision (words 23-26).
    for (uint16_t i = 0; i < ATA_FIRMWARE_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(channel, &checksum);
        result.firmwareRevision[i] = (char)(value >> 8);
        result.firmwareRevision[i+1] = (char)(value & 0xFF);
    }
    result.firmwareRevision[ATA_FIRMWARE_LENGTH] = '\0';

    // Read Model (words 27-46).
    for (uint16_t i = 0; i < ATA_MODEL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(channel, &checksum);
        result.model[i] = (char)(value >> 8);
        result.model[i+1] = (char)(value & 0xFF);
    }
    result.model[ATA_MODEL_LENGTH] = '\0';

    // Read words 47-59.
    result.maxSectorsInterrupt = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    result.trustedComputingFlags = ata_read_identify_word(channel, &checksum);
    result.capabilities49 = ata_read_identify_word(channel, &checksum);
    result.capabilities50 = ata_read_identify_word(channel, &checksum);
    result.pioMode = (uint8_t)(ata_read_identify_word(channel, &checksum) >> 8);
    ata_read_identify_word(channel, &checksum);
    result.flags53 = ata_read_identify_word(channel, &checksum);
    result.currentLogicalCylinders = ata_read_identify_word(channel, &checksum);
    result.currentLogicalHeads = ata_read_identify_word(channel, &checksum);
    result.currentLogicalSectorsPerTrack = ata_read_identify_word(channel, &checksum);
    result.currentCapacitySectors = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    result.flags59 = ata_read_identify_word(channel, &checksum);

    // Read words 60-79.
    result.totalLba28Bit = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    ata_read_identify_word(channel, &checksum);
    result.multiwordDmaFlags = ata_read_identify_word(channel, &checksum);
    result.pioModesSupported = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    result.multiwordDmaMinCycleTime = ata_read_identify_word(channel, &checksum);
    result.multiwordDmaRecCycleTime = ata_read_identify_word(channel, &checksum);
    result.pioMinCycleTimeNoFlow = ata_read_identify_word(channel, &checksum);
    result.pioMinCycleTimeIoRdy = ata_read_identify_word(channel, &checksum);
    result.additionalSupportedFlags = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 70, 74);
    result.maxQueueDepth = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0x1F);
    result.serialAtaFlags76 = ata_read_identify_word(channel, &checksum);
    ata_read_identify_word(channel, &checksum);
    result.serialAtaFlags78 = ata_read_identify_word(channel, &checksum);
    result.serialAtaFlags79 = ata_read_identify_word(channel, &checksum);

    // Read words 80-93.
    result.versionMajor = ata_read_identify_word(channel, &checksum);
    result.versionMinor = ata_read_identify_word(channel, &checksum);
    result.commandFlags82 = ata_read_identify_word(channel, &checksum);
    result.commandFlags83.data = ata_read_identify_word(channel, &checksum);
    result.commandFlags84 = ata_read_identify_word(channel, &checksum);
    result.commandFlags85 = ata_read_identify_word(channel, &checksum);
    result.commandFlags86 = ata_read_identify_word(channel, &checksum);
    result.commandFlags87 = ata_read_identify_word(channel, &checksum);
    result.ultraDmaMode = ata_read_identify_word(channel, &checksum);
    result.normalEraseTime = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    result.enhancedEraseTime = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    result.currentApmLevel = ata_read_identify_word(channel, &checksum);
    result.masterPasswordRevision = ata_read_identify_word(channel, &checksum);
    result.hardwareResetResult = ata_read_identify_word(channel, &checksum);

    // Read acoustic values (word 94).
    uint16_t acoustic = ata_read_identify_word(channel, &checksum);
    result.recommendedAcousticValue = (uint8_t)(acoustic >> 8);
    result.currentAcousticValue = (uint8_t)(acoustic & 0xFF);

    // Read stream values (words 95-99).
    result.streamMinSize = ata_read_identify_word(channel, &checksum);
    result.streamTransferTime = ata_read_identify_word(channel, &checksum);
    result.streamTransferTimePio = ata_read_identify_word(channel, &checksum);
    result.streamAccessLatency = ata_read_identify_word(channel, &checksum);
    result.streamPerfGranularity = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);

    // Read total sectors for 48-bit LBA (words 100-103).
    result.totalLba48Bit = (uint64_t)ata_read_identify_word(channel, &checksum) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(channel, &checksum) << 32) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 48);

    // Read words 104-107.
    result.streamTransferTimePio = ata_read_identify_word(channel, &checksum);
    ata_read_identify_word(channel, &checksum);
    result.physicalSectorSize = ata_read_identify_word(channel, &checksum);
    result.interSeekDelay = ata_read_identify_word(channel, &checksum);

    // Read world wide name (words 108-111).
    result.worldWideName = (uint64_t)ata_read_identify_word(channel, &checksum) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(channel, &checksum) << 32) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 48);

    // Read words 112-128.
    ata_read_identify_words(channel, &checksum, 112, 116);
    result.logicalSectorSize = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    result.commandFlags119 = ata_read_identify_word(channel, &checksum);
    result.commandFlags120 = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 121, 126);
    result.removableMediaFlags = ata_read_identify_word(channel, &checksum);
    result.securityFlags = ata_read_identify_word(channel, &checksum);

    // Read unused words 129-159.
    ata_read_identify_words(channel, &checksum, 129, 159);

    // Read words 160-169.
    result.cfaFlags = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 161, 167);
    result.formFactor = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xF);
    result.dataSetManagementFlags = ata_read_identify_word(channel, &checksum);

    // Read additional identifier (words 170-173).
    for (uint16_t i = 0; i < ATA_ADD_ID_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(channel, &checksum);
        result.additionalIdentifier[i] = (char)(value >> 8);
        result.additionalIdentifier[i+1] = (char)(value & 0xFF);
    }
    result.additionalIdentifier[ATA_ADD_ID_LENGTH] = '\0';

    // Read unused words 174 and 175.
    ata_read_identify_words(channel, &checksum, 174, 175);

    // Read current media serial number (words 176-205).
    for (uint16_t i = 0; i < ATA_MEDIA_SERIAL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(channel, &checksum);
        result.mediaSerial[i] = (char)(value >> 8);
        result.mediaSerial[i+1] = (char)(value & 0xFF);
    }
    result.mediaSerial[ATA_MEDIA_SERIAL_LENGTH] = '\0';

    // Read words 206-219.
    result.sctCommandTransportFlags = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 207, 208);
    result.physicalBlockAlignment = ata_read_identify_word(channel, &checksum);
    result.writeReadVerifySectorCountMode3 = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    result.writeReadVerifySectorCountMode2 = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    result.nvCacheCapabilities = ata_read_identify_word(channel, &checksum);
    result.nvCacheSize = (uint32_t)ata_read_identify_word(channel, &checksum) | ((uint32_t)ata_read_identify_word(channel, &checksum) << 16);
    result.rotationRate = ata_read_identify_word(channel, &checksum);
    ata_read_identify_word(channel, &checksum);
    result.nvCacheFlags = ata_read_identify_word(channel, &checksum);

    // Read words 220-254.
    result.writeReadVerifyCurrentMode = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    ata_read_identify_word(channel, &checksum);
    result.transportVersionMajor = ata_read_identify_word(channel, &checksum);
    result.transportVersionMinor = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 224, 229);
    result.extendedSectors = (uint64_t)ata_read_identify_word(channel, &checksum) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(channel, &checksum) << 32) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 48);
    ata_read_identify_words(channel, &checksum, 234, 254);

    // Read integrity word 255.
    // If the low byte contains the magic number, validate checksum. If check fails, command failed.
    uint16_t integrity = ata_read_identify_word(channel, &checksum);
    if (((uint8_t)(integrity & 0xFF)) == ATA_IDENTIFY_INTEGRITY_MAGIC && checksum != 0)
        return ATA_CHK_STATUS_ERROR;

    // Ensure device is in fact an ATA device.
    if (result.generalConfig & ATA_IDENTIFY_GENERAL_NOT_ATA_DEVICE)
        return ATA_CHK_STATUS_ERROR;

    // Ensure outputs are good.

    // Command succeeded.
    *outResult = result;
    return ata_check_status(channel, master);
}

int16_t ata_read_sector(ata_channel_t *channel, bool master, uint32_t startSectorLba, void *outData, uint8_t sectorCount) {
    // Send READ SECTOR command.
    ata_set_lba_high(channel, (uint8_t)((startSectorLba >> 24) & 0x0F));
    ata_send_command(channel, sectorCount, (uint8_t)(startSectorLba & 0xFF),
        (uint8_t)((startSectorLba >> 8) & 0xFF), (uint8_t)((startSectorLba >> 16) & 0xFF), ATA_CMD_READ_SECTOR);

    // Wait for device.
    if (!ata_wait_for_drq(channel))
       return ata_check_status(channel, master);

    // Read data.
    ata_read_data_pio(channel, outData, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
    return ata_check_status(channel, master);
}

int16_t ata_read_sector_ext(ata_channel_t *channel, bool master, uint64_t startSectorLba, void *outData, uint16_t sectorCount) {
    // Get low and high parts of 48-bit LBA address.
    uint32_t lbaLow = (uint32_t)(startSectorLba & 0xFFFFFFFF);
    uint32_t lbaHigh = (uint32_t)(startSectorLba >> 32);

    // Send high LBA bytes, then low LBA bytes and command.
    ata_set_lba_high(channel, (uint8_t)((lbaHigh >> 24) & 0x0F));
    ata_send_params(channel, (uint8_t)(sectorCount >> 8), (uint8_t)(lbaHigh & 0xFF),
        (uint8_t)((lbaHigh >> 8) & 0xFF), (uint8_t)((lbaHigh >> 16) & 0xFF));
    ata_set_lba_high(channel, (uint8_t)((lbaLow >> 24) & 0x0F));
    ata_send_command(channel, (uint8_t)(sectorCount & 0xFF), (uint8_t)(lbaLow & 0xFF),
        (uint8_t)((lbaLow >> 8) & 0xFF), (uint8_t)((lbaLow >> 16) & 0xFF), ATA_CMD_READ_SECTOR_EXT);

    // Wait for device.
    if (!ata_wait_for_drq(channel))
       return ata_check_status(channel, master);

    // Read data.
    ata_read_data_pio(channel, outData, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
    return ata_check_status(channel, master);
}

int16_t ata_write_sector(ata_channel_t *channel, bool master, uint32_t startSectorLba, const void *data, uint8_t sectorCount) {
    ata_check_status(channel, master);

    // Send WRITE SECTOR command.
    ata_set_lba_high(channel, (uint8_t)((startSectorLba >> 24) & 0x0F));
    ata_send_command(channel, sectorCount, (uint8_t)(startSectorLba & 0xFF),
        (uint8_t)((startSectorLba >> 8) & 0xFF), (uint8_t)((startSectorLba >> 16) & 0xFF), ATA_CMD_WRITE_SECTOR);

    // Wait for device.
    if (!ata_wait_for_drq(channel))
       return ata_check_status(channel, master);

    // Read data.
    ata_write_data_pio(channel, data, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
    return ata_check_status(channel, master);
}
