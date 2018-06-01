/*
 * File: atapi_commands.c
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


int16_t ata_identify_packet(ata_channel_t *channel, bool master, ata_identify_packet_result_t *outResult) {
    // Send IDENTIFY PACKET command.
    ata_send_command(channel, 0x00, 0x00, 0x00, 0x00, ATA_CMD_IDENTIFY_PACKET);

    // Wait for device.
    if (ata_wait_for_irq(channel, master) != ATA_CHK_STATUS_OK || !ata_wait_for_drq(channel))
       return ata_check_status(channel, master);

    // Checksum total, used at end.
    uint8_t checksum = 0;
    ata_identify_packet_result_t result = {};

    // Read words 0-9.
    result.generalConfig.data = ata_read_identify_word(channel, &checksum);
    ata_read_identify_word(channel, &checksum);
    result.specificConfig = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 3, 9);

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

    // Read words 47-61.
    ata_read_identify_words(channel, &checksum, 47, 48);
    result.capabilities49 = ata_read_identify_word(channel, &checksum);
    result.capabilities50 = ata_read_identify_word(channel, &checksum);
    result.pioMode = (uint8_t)(ata_read_identify_word(channel, &checksum) >> 8);
    ata_read_identify_word(channel, &checksum);
    result.flags53 = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 54, 61);

    // Read words 62-79.
    result.dmaFlags62 = ata_read_identify_word(channel, &checksum);
    result.multiwordDmaFlags = ata_read_identify_word(channel, &checksum);
    result.pioModesSupported = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0xFF);
    result.multiwordDmaMinCycleTime = ata_read_identify_word(channel, &checksum);
    result.multiwordDmaRecCycleTime = ata_read_identify_word(channel, &checksum);
    result.pioMinCycleTimeNoFlow = ata_read_identify_word(channel, &checksum);
    result.pioMinCycleTimeIoRdy = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 69, 70);
    result.timePacketRelease = ata_read_identify_word(channel, &checksum);
    result.timeServiceBusy = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 73, 74);
    result.maxQueueDepth = (uint8_t)(ata_read_identify_word(channel, &checksum) & 0x1F);
    result.serialAtaFlags76 = ata_read_identify_word(channel, &checksum);
    ata_read_identify_word(channel, &checksum);
    result.serialAtaFlags78 = ata_read_identify_word(channel, &checksum);
    result.serialAtaFlags79 = ata_read_identify_word(channel, &checksum);

    // Read words 80-93.
    result.versionMajor = ata_read_identify_word(channel, &checksum);
    result.versionMinor = ata_read_identify_word(channel, &checksum);
    result.commandFlags82 = ata_read_identify_word(channel, &checksum);
    result.commandFlags83 = ata_read_identify_word(channel, &checksum);
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

    // Read unused words 95-107.
    ata_read_identify_words(channel, &checksum, 95, 107);

    // Read world wide name (words 108-111).
    result.worldWideName = (uint64_t)ata_read_identify_word(channel, &checksum) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 16)
        | ((uint64_t)ata_read_identify_word(channel, &checksum) << 32) | ((uint64_t)ata_read_identify_word(channel, &checksum) << 48);

    // Read words 112-128.
    ata_read_identify_words(channel, &checksum, 112, 118);
    result.commandFlags119 = ata_read_identify_word(channel, &checksum);
    result.commandFlags120 = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 121, 126);
    result.removableMediaFlags = ata_read_identify_word(channel, &checksum);
    result.securityFlags = ata_read_identify_word(channel, &checksum);

    // Read unused words 129-221.
    ata_read_identify_words(channel, &checksum, 129, 221);

    // Read words 220-254.
    result.transportVersionMajor = ata_read_identify_word(channel, &checksum);
    result.transportVersionMinor = ata_read_identify_word(channel, &checksum);
    ata_read_identify_words(channel, &checksum, 224, 254);

    // Read integrity word 255.
    // If the low byte contains the magic number, validate checksum. If check fails, command failed.
    uint16_t integrity = ata_read_identify_word(channel, &checksum);
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

/*bool ata_packet(uint16_t portCommand, uint16_t portControl, bool master, void *packet, uint16_t packetSize, uint16_t devicePacketSize, void *outResult, uint16_t resultSize) {
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

   /* ata_check_status(portCommand, master);
    intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
    kprintf("ATA: int 0x%X\n", intReason);


    kprintf("ATA: Error 0x%X\n", inb(ATA_REG_ERROR(portCommand)));
    kprintf("ATA: Status: 0x%X\n", inb(ATA_REG_ALT_STATUS(portControl)));
    kprintf("ATA: Low: 0x%X\n", inb(ATA_REG_CYLINDER_LOW(portCommand)));
    kprintf("ATA: High: 0x%X\n", inb(ATA_REG_CYLINDER_HIGH(portCommand)));
    //intReason = inb(ATA_REG_SECTOR_COUNT(portCommand));
   // kprintf("ATA: int 0x%X\n", intReason);
}*/
