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
#include <math.h>
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

    // Read identify result.
    ata_identify_result_t result;
    ata_read_data_pio(channel, sizeof(ata_identify_result_t), &result, sizeof(ata_identify_result_t));

    // Checksum total, used at end.
    uint8_t checksum = 0;

    // Read integrity word 255.
    // If the low byte contains the magic number, validate checksum. If check fails, command failed.
    /*uint16_t integrity = ata_read_identify_word(channel, &checksum);
    if (((uint8_t)(integrity & 0xFF)) == ATA_IDENTIFY_INTEGRITY_MAGIC && checksum != 0)
        return ATA_CHK_STATUS_ERROR;

    // Ensure device is in fact an ATA device.
    if (result.generalConfig & ATA_IDENTIFY_GENERAL_NOT_ATA_DEVICE)
        return ATA_CHK_STATUS_ERROR;*/

    // Ensure outputs are good.

    // Command succeeded.
    *outResult = result;
    return ata_check_status(channel, master);
}

int16_t ata_read_sector(ata_device_t *ataDevice, uint64_t startSectorLba, void *outData, uint32_t length) {
    // Get sectors.
    uint32_t sectorCount = DIVIDE_ROUND_UP(length, ataDevice->BytesPerSector);

    // If above 256 sectors, cap at 256 sectors.
    if (sectorCount > 256) {
        sectorCount = 0;
        length = ataDevice->BytesPerSector * 256;
    }
    else if (sectorCount == 256) {
        // If 256 sectors, sector count is to be zero.
        sectorCount = 0;
    }

    ata_check_status(ataDevice->Channel, ataDevice->Master);

    // Send READ SECTOR command.
    ata_set_lba_high(ataDevice->Channel, (uint8_t)((startSectorLba >> 24) & 0x0F));
    ata_send_command(ataDevice->Channel, (uint8_t)sectorCount, (uint8_t)(startSectorLba & 0xFF),
        (uint8_t)((startSectorLba >> 8) & 0xFF), (uint8_t)((startSectorLba >> 16) & 0xFF), ATA_CMD_READ_SECTOR);

    // Wait for device.
    if (!ata_wait_for_drq(ataDevice->Channel))
       return ata_check_status(ataDevice->Channel, ataDevice->Master);

    // Read data.
    ata_read_data_pio(ataDevice->Channel, (sectorCount == 0 ? 256 : sectorCount) * ataDevice->BytesPerSector, outData, length);

    uint16_t i = 0;
    while (inb(ATA_REG_STATUS(((ata_channel_t*)ataDevice->Channel)->CommandPort)) & ATA_STATUS_DATA_REQUEST) {
        inw(ATA_REG_DATA(((ata_channel_t*)ataDevice->Channel)->CommandPort));
        i++;
    }

    return ata_check_status(ataDevice->Channel, ataDevice->Master);
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
    //ata_read_data_pio(channel, outData, (sectorCount == 0 ? 256 : sectorCount) * ATA_SECTOR_SIZE_512);
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
