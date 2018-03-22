#include <main.h>
#include <io.h>
#include <driver/ata/ata.h>
#include <driver/ata/ata_commands.h>


extern bool ata_wait_for_irq_pri(uint16_t portCommand, bool master);
extern bool ata_wait_for_irq_sec(uint16_t portCommand, bool master);


static uint16_t ata_read_identify_word(uint16_t portCommand, uint8_t *checksum) {
    // Read word, adding value to checksum.
    uint16_t data = ata_read_data_word(portCommand);
    *checksum += (uint8_t)(data >> 8);
    *checksum += (uint8_t)(data & 0xFF);
    return data;
}

static void ata_read_identify_words(uint16_t portCommand, uint8_t *checksum, uint8_t firstWord, uint8_t lastWord) {
    for (uint8_t word = firstWord; word <= lastWord; word++)
        ata_read_identify_word(portCommand, checksum);
}

bool ata_identify(uint16_t portCommand, uint16_t portControl, bool master, ata_identify_result_t *outResult) {
    // Send identify command and wait for IRQ.
    ata_send_command(portCommand, 0x00, 0x00, 0x00, 0x00, ATA_CMD_IDENTIFY);
    if (portCommand == ATA_PRI_COMMAND_PORT && !ata_wait_for_irq_pri(portCommand, master))
        return false;
    else if (portCommand == ATA_SEC_COMMAND_PORT && !ata_wait_for_irq_sec(portCommand, master))
        return false;

    // Ensure drive is ready and there is data to be read.
    if (!ata_check_status(portCommand, master) || !ata_data_ready(portCommand))
        return false;

    // Checksum total, used at end.
    uint8_t checksum = 0;
    ata_identify_result_t result = {};

    // Read words 0-9.
    result.generalConfig = ata_read_identify_word(portCommand, &checksum);
    result.logicalCylinders = ata_read_identify_word(portCommand, &checksum);
    result.specificConfig = ata_read_identify_word(portCommand, &checksum);
    result.logicalHeads = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 4, 5);
    result.logicalSectorsPerTrack = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 7, 9);

    // Read serial number (words 10-19).
    for (uint16_t i = 0; i < ATA_SERIAL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.serial[i] = (char)(value >> 8);
        result.serial[i+1] = (char)(value & 0xFF);
    }
    result.serial[ATA_SERIAL_LENGTH] = '\0';

    // Unused words 20-22.
    ata_read_identify_words(portCommand, &checksum, 20, 22);

    // Read firmware revision (words 23-26).
    for (uint16_t i = 0; i < ATA_FIRMWARE_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.firmwareRevision[i] = (char)(value >> 8);
        result.firmwareRevision[i+1] = (char)(value & 0xFF);
    }
    result.firmwareRevision[ATA_FIRMWARE_LENGTH] = '\0';

    // Read Model (words 27-46).
    for (uint16_t i = 0; i < ATA_MODEL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.model[i] = (char)(value >> 8);
        result.model[i+1] = (char)(value & 0xFF);
    }
    result.model[ATA_MODEL_LENGTH] = '\0';

    // Read words 47-59.
    result.maxSectorsInterrupt = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.trustedComputingFlags = ata_read_identify_word(portCommand, &checksum);
    result.capabilities49 = ata_read_identify_word(portCommand, &checksum);
    result.capabilities50 = ata_read_identify_word(portCommand, &checksum);
    result.pioMode = (uint8_t)(ata_read_identify_word(portCommand, &checksum) >> 8);
    ata_read_identify_word(portCommand, &checksum);
    result.flags53 = ata_read_identify_word(portCommand, &checksum);
    result.currentLogicalCylinders = ata_read_identify_word(portCommand, &checksum);
    result.currentLogicalHeads = ata_read_identify_word(portCommand, &checksum);
    result.currentLogicalSectorsPerTrack = ata_read_identify_word(portCommand, &checksum);
    result.currentCapacitySectors = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    result.flags59 = ata_read_identify_word(portCommand, &checksum);

    // Read words 60-79.
    result.totalLba28Bit = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    ata_read_identify_word(portCommand, &checksum);
    result.multiwordDmaFlags = ata_read_identify_word(portCommand, &checksum);
    result.pioModesSupported = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.multiwordDmaMinCycleTime = ata_read_identify_word(portCommand, &checksum);
    result.multiwordDmaRecCycleTime = ata_read_identify_word(portCommand, &checksum);
    result.pioMinCycleTimeNoFlow = ata_read_identify_word(portCommand, &checksum);
    result.pioMinCycleTimeIoRdy = ata_read_identify_word(portCommand, &checksum);
    result.additionalSupportedFlags = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 70, 74);
    result.maxQueueDepth = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0x1F);
    result.serialAtaFlags76 = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_word(portCommand, &checksum);
    result.serialAtaFlags78 = ata_read_identify_word(portCommand, &checksum);
    result.serialAtaFlags79 = ata_read_identify_word(portCommand, &checksum);

    // Read words 80-93.
    result.versionMajor = ata_read_identify_word(portCommand, &checksum);
    result.versionMinor = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags82 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags83 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags84 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags85 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags86 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags87 = ata_read_identify_word(portCommand, &checksum);
    result.ultraDmaMode = ata_read_identify_word(portCommand, &checksum);
    result.normalEraseTime = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.enhancedEraseTime = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.currentApmLevel = ata_read_identify_word(portCommand, &checksum);
    result.masterPasswordRevision = ata_read_identify_word(portCommand, &checksum);
    result.hardwareResetResult = ata_read_identify_word(portCommand, &checksum);

    // Read acoustic values (word 94).
    uint16_t acoustic = ata_read_identify_word(portCommand, &checksum);
    result.recommendedAcousticValue = (uint8_t)(acoustic >> 8);
    result.currentAcousticValue = (uint8_t)(acoustic & 0xFF);

    // Read stream values (words 95-99).
    result.streamMinSize = ata_read_identify_word(portCommand, &checksum);
    result.streamTransferTime = ata_read_identify_word(portCommand, &checksum);
    result.streamTransferTimePio = ata_read_identify_word(portCommand, &checksum);
    result.streamAccessLatency = ata_read_identify_word(portCommand, &checksum);
    result.streamPerfGranularity = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);

    // Read total sectors for 48-bit LBA (words 100-103).
    result.totalLba48Bit = (uint64_t)ata_read_identify_word(portCommand, &checksum) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 32) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 48);

    // Read words 104-107.
    result.streamTransferTimePio = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_word(portCommand, &checksum);
    result.physicalSectorSize = ata_read_identify_word(portCommand, &checksum);
    result.interSeekDelay = ata_read_identify_word(portCommand, &checksum);

    // Read world wide name (words 108-111).
    result.worldWideName = (uint64_t)ata_read_identify_word(portCommand, &checksum) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 32) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 48);

    // Read words 112-128.
    ata_read_identify_words(portCommand, &checksum, 112, 116);
    result.logicalSectorSize = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    result.commandFlags119 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags120 = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 121, 126);
    result.removableMediaFlags = ata_read_identify_word(portCommand, &checksum);
    result.securityFlags = ata_read_identify_word(portCommand, &checksum);

    // Read unused words 129-159.
    ata_read_identify_words(portCommand, &checksum, 129, 159);

    // Read words 160-169.
    result.cfaFlags = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 161, 167);
    result.formFactor = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xF);
    result.dataSetManagementFlags = ata_read_identify_word(portCommand, &checksum);

    // Read additional identifier (words 170-173).
    for (uint16_t i = 0; i < ATA_ADD_ID_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.additionalIdentifier[i] = (char)(value >> 8);
        result.additionalIdentifier[i+1] = (char)(value & 0xFF);
    }
    result.additionalIdentifier[ATA_ADD_ID_LENGTH] = '\0';

    // Read unused words 174 and 175.
    ata_read_identify_words(portCommand, &checksum, 174, 175);

    // Read current media serial number (words 176-205).
    for (uint16_t i = 0; i < ATA_MEDIA_SERIAL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.mediaSerial[i] = (char)(value >> 8);
        result.mediaSerial[i+1] = (char)(value & 0xFF);
    }
    result.mediaSerial[ATA_MEDIA_SERIAL_LENGTH] = '\0';

    // Read words 206-219.
    result.sctCommandTransportFlags = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 207, 208);
    result.physicalBlockAlignment = ata_read_identify_word(portCommand, &checksum);
    result.writeReadVerifySectorCountMode3 = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    result.writeReadVerifySectorCountMode2 = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    result.nvCacheCapabilities = ata_read_identify_word(portCommand, &checksum);
    result.nvCacheSize = (uint32_t)ata_read_identify_word(portCommand, &checksum) | ((uint32_t)ata_read_identify_word(portCommand, &checksum) << 16);
    result.rotationRate = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_word(portCommand, &checksum);
    result.nvCacheFlags = ata_read_identify_word(portCommand, &checksum);

    // Read words 220-254.
    result.writeReadVerifyCurrentMode = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    ata_read_identify_word(portCommand, &checksum);
    result.transportVersionMajor = ata_read_identify_word(portCommand, &checksum);
    result.transportVersionMinor = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 224, 229);
    result.extendedSectors = (uint64_t)ata_read_identify_word(portCommand, &checksum) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 32) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 48);
    ata_read_identify_words(portCommand, &checksum, 234, 254);

    // Read integrity word 255.
    // If the low byte contains the magic number, validate checksum. If check fails, command failed.
    uint16_t integrity = ata_read_identify_word(portCommand, &checksum);
    if (((uint8_t)(integrity & 0xFF)) == ATA_IDENTIFY_INTEGRITY_MAGIC && checksum != 0)
        return false;

    // Ensure device is in fact an ATA device.
    if (result.generalConfig & ATA_IDENTIFY_GENERAL_NOT_ATA_DEVICE)
        return false;

    // Ensure outputs are good.

    // Command succeeded.
    *outResult = result;
    return true;
}

bool ata_identify_packet(uint16_t portCommand, uint16_t portControl, bool master, ata_identify_packet_result_t *outResult) {
    // Send identify command and wait for IRQ.
    ata_send_command(portCommand, 0x00, 0x00, 0x00, 0x00, ATA_CMD_IDENTIFY_PACKET);
    if (portCommand == ATA_PRI_COMMAND_PORT && !ata_wait_for_irq_pri(portCommand, master))
        return false;
    else if (portCommand == ATA_SEC_COMMAND_PORT && !ata_wait_for_irq_sec(portCommand, master))
        return false;

    // Ensure drive is ready and there is data to be read.
    if (!ata_check_status(portCommand, master) || !ata_data_ready(portCommand))
        return false;

    // Checksum total, used at end.
    uint8_t checksum = 0;
    ata_identify_packet_result_t result = {};

    // Read words 0-9.
    result.generalConfig.data = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_word(portCommand, &checksum);
    result.specificConfig = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 3, 9);

    // Read serial number (words 10-19).
    for (uint16_t i = 0; i < ATA_SERIAL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.serial[i] = (char)(value >> 8);
        result.serial[i+1] = (char)(value & 0xFF);
    }
    result.serial[ATA_SERIAL_LENGTH] = '\0';

    // Unused words 20-22.
    ata_read_identify_words(portCommand, &checksum, 20, 22);

    // Read firmware revision (words 23-26).
    for (uint16_t i = 0; i < ATA_FIRMWARE_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.firmwareRevision[i] = (char)(value >> 8);
        result.firmwareRevision[i+1] = (char)(value & 0xFF);
    }
    result.firmwareRevision[ATA_FIRMWARE_LENGTH] = '\0';

    // Read Model (words 27-46).
    for (uint16_t i = 0; i < ATA_MODEL_LENGTH; i+=2) {
        uint16_t value = ata_read_identify_word(portCommand, &checksum);
        result.model[i] = (char)(value >> 8);
        result.model[i+1] = (char)(value & 0xFF);
    }
    result.model[ATA_MODEL_LENGTH] = '\0';

    // Read words 47-61.
    ata_read_identify_words(portCommand, &checksum, 47, 48);
    result.capabilities49 = ata_read_identify_word(portCommand, &checksum);
    result.capabilities50 = ata_read_identify_word(portCommand, &checksum);
    result.pioMode = (uint8_t)(ata_read_identify_word(portCommand, &checksum) >> 8);
    ata_read_identify_word(portCommand, &checksum);
    result.flags53 = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 54, 61);

    // Read words 62-79.
    result.dmaFlags62 = ata_read_identify_word(portCommand, &checksum);
    result.multiwordDmaFlags = ata_read_identify_word(portCommand, &checksum);
    result.pioModesSupported = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.multiwordDmaMinCycleTime = ata_read_identify_word(portCommand, &checksum);
    result.multiwordDmaRecCycleTime = ata_read_identify_word(portCommand, &checksum);
    result.pioMinCycleTimeNoFlow = ata_read_identify_word(portCommand, &checksum);
    result.pioMinCycleTimeIoRdy = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 69, 70);
    result.timePacketRelease = ata_read_identify_word(portCommand, &checksum);
    result.timeServiceBusy = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 73, 74);
    result.maxQueueDepth = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0x1F);
    result.serialAtaFlags76 = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_word(portCommand, &checksum);
    result.serialAtaFlags78 = ata_read_identify_word(portCommand, &checksum);
    result.serialAtaFlags79 = ata_read_identify_word(portCommand, &checksum);

    // Read words 80-93.
    result.versionMajor = ata_read_identify_word(portCommand, &checksum);
    result.versionMinor = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags82 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags83 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags84 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags85 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags86 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags87 = ata_read_identify_word(portCommand, &checksum);
    result.ultraDmaMode = ata_read_identify_word(portCommand, &checksum);
    result.normalEraseTime = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.enhancedEraseTime = (uint8_t)(ata_read_identify_word(portCommand, &checksum) & 0xFF);
    result.currentApmLevel = ata_read_identify_word(portCommand, &checksum);
    result.masterPasswordRevision = ata_read_identify_word(portCommand, &checksum);
    result.hardwareResetResult = ata_read_identify_word(portCommand, &checksum);

    // Read acoustic values (word 94).
    uint16_t acoustic = ata_read_identify_word(portCommand, &checksum);
    result.recommendedAcousticValue = (uint8_t)(acoustic >> 8);
    result.currentAcousticValue = (uint8_t)(acoustic & 0xFF);

    // Read unused words 95-107.
    ata_read_identify_words(portCommand, &checksum, 95, 107);

    // Read world wide name (words 108-111).
    result.worldWideName = (uint64_t)ata_read_identify_word(portCommand, &checksum) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 32) | ((uint64_t)ata_read_identify_word(portCommand, &checksum) << 48);

    // Read words 112-128.
    ata_read_identify_words(portCommand, &checksum, 112, 118);
    result.commandFlags119 = ata_read_identify_word(portCommand, &checksum);
    result.commandFlags120 = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 121, 126);
    result.removableMediaFlags = ata_read_identify_word(portCommand, &checksum);
    result.securityFlags = ata_read_identify_word(portCommand, &checksum);

    // Read unused words 129-221.
    ata_read_identify_words(portCommand, &checksum, 129, 221);

    // Read words 220-254.
    result.transportVersionMajor = ata_read_identify_word(portCommand, &checksum);
    result.transportVersionMinor = ata_read_identify_word(portCommand, &checksum);
    ata_read_identify_words(portCommand, &checksum, 224, 254);

    // Read integrity word 255.
    // If the low byte contains the magic number, validate checksum. If check fails, command failed.
    uint16_t integrity = ata_read_identify_word(portCommand, &checksum);
    if (((uint8_t)(integrity & 0xFF)) == ATA_IDENTIFY_INTEGRITY_MAGIC && checksum != 0)
        return false;

    // Ensure device is in fact an ATAPI device.
    if (!(result.generalConfig.data & ATA_IDENTIFY_GENERAL_ATAPI_DEVICE))
        return false;

    // Ensure outputs are good.

    // Command succeeded.
    *outResult = result;
    return true;
}

bool ata_packet(uint16_t portCommand, uint16_t portControl, bool master, void *packet, uint16_t packetSize, uint16_t devicePacketSize, void *outResult, uint16_t resultSize) {
    ata_check_status(portCommand, master);
    uint8_t intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
    kprintf("ATA: int 0x%X\n", intReason);

    // Send identify command and wait for DRQ.
    kprintf("ATA: Sending PACKET to %s on channel 0x%X...\n", master ? "master" : "slave", portCommand);
    outb(ATA_REG_FEATURES(portCommand), 0x00);
    ata_send_command(portCommand, 0x00, 0x00, (uint8_t)(packetSize & 0xFF), (uint8_t)(packetSize >> 8), ATA_CMD_PACKET);
        intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
    kprintf("ATA: int 0x%X\n", intReason);

    // Wait for device.
    if (!ata_wait_for_drq(portControl))
       return false;

    // Get packet.
    uint8_t fullPacket[devicePacketSize];
    memset(fullPacket, 0, devicePacketSize);
    memcpy(fullPacket, packet, packetSize);


    // Send packet.
    //uint8_t packet[6] = { 0x12, 0, 0, 0, 96, 0};
   // uint16_t *packetWords = (uint16_t*)packet;

    kprintf("ATA: Sending command packet 0x%X...\n", fullPacket[0]);
    ata_write_data_pio(portCommand, fullPacket, devicePacketSize);
   // outw(ATA_REG_DATA(portCommand), packetWords[0]);
   // outw(ATA_REG_DATA(portCommand), packetWords[1]);
  //  outw(ATA_REG_DATA(portCommand), packetWords[2]);

    // Pad remaining 
  //  outw(ATA_REG_DATA(portCommand), 0x0);
  //  outw(ATA_REG_DATA(portCommand), 0x0);
  //  outw(ATA_REG_DATA(portCommand), 0x0);
   // outb(ATA_REG_DATA(portCommand), packet[3]);
    //outb(ATA_REG_DATA(portCommand), packet[4]);
    //outb(ATA_REG_DATA(portCommand), packet[5]);
    //outb(ATA_REG_DATA(portCommand), packet[0]);

    // Wait for IRQ.
    if (portCommand == ATA_PRI_COMMAND_PORT && !ata_wait_for_irq_pri(portCommand, master))
        return false;
    else if (portCommand == ATA_SEC_COMMAND_PORT && !ata_wait_for_irq_sec(portCommand, master))
        return false;

    ata_check_status(portCommand, master);
    intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
    kprintf("ATA: int 0x%X\n", intReason);
    kprintf("ATA: Error 0x%X\n", inb(ATA_REG_ERROR(portCommand)));
    kprintf("ATA: Status: 0x%X\n", inb(ATA_REG_ALT_STATUS(portControl)));
    kprintf("ATA: Low: 0x%X\n", inb(ATA_REG_CYLINDER_LOW(portCommand)));
    kprintf("ATA: High: 0x%X\n", inb(ATA_REG_CYLINDER_HIGH(portCommand)));

    if (!ata_wait_for_drq(portControl))
        return false;

    kprintf("ATA: Getting result of command 0x%X...\n", fullPacket[0]);
    ata_read_data_pio(portCommand, outResult, resultSize);

    //kprintf("ATA: Getting data...\n");
    /*uint8_t data[96];
    uint16_t *datPr = (uint16_t*)data;

    for (uint16_t i = 0; i < 96; i+= 2) {
        datPr[i / 2] = inw(ATA_REG_DATA(portCommand));
    }*/

    ata_check_status(portCommand, master);
    intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
    kprintf("ATA: int 0x%X\n", intReason);


    kprintf("ATA: Error 0x%X\n", inb(ATA_REG_ERROR(portCommand)));
    kprintf("ATA: Status: 0x%X\n", inb(ATA_REG_ALT_STATUS(portControl)));
    kprintf("ATA: Low: 0x%X\n", inb(ATA_REG_CYLINDER_LOW(portCommand)));
    kprintf("ATA: High: 0x%X\n", inb(ATA_REG_CYLINDER_HIGH(portCommand)));
    //intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
   // kprintf("ATA: int 0x%X\n", intReason);
}

bool ata_read_sector(uint16_t portCommand, uint16_t portControl, bool master, uint32_t startSectorLba, void *outData, uint8_t sectorCount) {
    // Send READ SECTOR command.
    ata_set_lba_high(portCommand, (uint8_t)((startSectorLba >> 24) & 0x0F));
    ata_send_command(portCommand, sectorCount, (uint8_t)(startSectorLba & 0xFF),
        (uint8_t)((startSectorLba >> 8) & 0xFF), (uint8_t)((startSectorLba >> 16) & 0xFF), ATA_CMD_READ_SECTOR);

    // Wait for device.
    if (!ata_wait_for_drq(portControl))
       return false;

    // Read data.
    ata_read_data_pio(portCommand, outData, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
    return ata_check_status(portCommand, master);
}

bool ata_write_sector(uint16_t portCommand, uint16_t portControl, bool master, uint32_t startSectorLba, const void *data, uint8_t sectorCount) {
    ata_check_status(portCommand, master);

    // Send WRITE SECTOR command.
    ata_set_lba_high(portCommand, (uint8_t)((startSectorLba >> 24) & 0x0F));
    ata_send_command(portCommand, sectorCount, (uint8_t)(startSectorLba & 0xFF),
        (uint8_t)((startSectorLba >> 8) & 0xFF), (uint8_t)((startSectorLba >> 16) & 0xFF), ATA_CMD_WRITE_SECTOR);

    // Wait for device.
    if (!ata_wait_for_drq(portControl))
       return false;

    // Read data.
    ata_write_data_pio(portCommand, data, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
    return ata_check_status(portCommand, master);
}
