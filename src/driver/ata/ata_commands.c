#include <main.h>
#include <io.h>
#include <driver/ata/ata.h>
#include <driver/ata/ata_commands.h>


extern irqTriggeredPri;
static bool ata_wait_for_irq_pri() {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 200;
	uint8_t ret = false;
	while (!irqTriggeredPri) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?
	if(irqTriggeredPri)
		ret = true;
	else
		kprintf("PATA: IRQ timeout for primary channel!\n");

	// Reset triggered value.
	irqTriggeredPri = false;
	return ret;
}

extern irqTriggeredSec;
static bool ata_wait_for_irq_sec() {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 200;
	uint8_t ret = false;
	while (!irqTriggeredSec) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?
	if(irqTriggeredSec)
		ret = true;
	else
		kprintf("PATA: IRQ timeout for secondary channel!\n");

	// Reset triggered value.
	irqTriggeredSec = false;
	return ret;
}

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
    // Select device.
    ata_select_device(portCommand, portControl, false, master);

    // Send identify command and wait for IRQ.
    outb(ATA_REG_COMMAND(portCommand), ATA_CMD_IDENTIFY);
    if (portCommand == ATA_PRI_COMMAND_PORT && !ata_wait_for_irq_pri())
        return false;
    else if (portCommand == ATA_SEC_COMMAND_PORT && !ata_wait_for_irq_sec())
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
    ata_read_identify_words(portCommand, &checksum, 130, 159);

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
