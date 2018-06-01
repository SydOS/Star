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
#include <string.h>
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

int16_t ata_identify(ata_device_t *ataDevice, ata_identify_result_t *outResult) {
    // Check status.
    int16_t status = ata_check_status(ataDevice->Channel, ataDevice->Master);
    if (status != ATA_CHK_STATUS_OK)
        return status;

    // Send IDENTIFY command.
    outb(ATA_REG_COMMAND(ataDevice->Channel->CommandPort), ATA_CMD_IDENTIFY);

    // Wait for device.
    if (ata_wait_for_irq(ataDevice->Channel, ataDevice->Master) != ATA_CHK_STATUS_OK || !ata_wait_for_drq(ataDevice->Channel))
        return ata_check_status(ataDevice->Channel, ataDevice->Master);

    // Read identify result.
    ata_identify_result_t result;
    ata_read_data_pio(ataDevice->Channel, sizeof(ata_identify_result_t), &result, sizeof(ata_identify_result_t));

    // Is the checksum valid?
    if (result.ChecksumValidityIndicator == ATA_IDENTIFY_INTEGRITY_MAGIC) {
        // Determine sum of bytes.
        uint8_t checksum = 0;
        uint8_t *resultBytes = (uint8_t*)&result;
        for (uint16_t i = 0; i < sizeof(ata_identify_result_t); i++)
            checksum += resultBytes[i];

        // If sum is not zero, data is damaged.
        if (checksum != 0)
            return ATA_CHK_STATUS_ERROR;
    }

    // Command succeeded.
    *outResult = result;
    return ata_check_status(ataDevice->Channel, ataDevice->Master);
}

int16_t ata_read_sector(ata_device_t *ataDevice, uint64_t startSectorLba, void *outData, uint32_t length) {
    // Check status.
    int16_t status = ata_check_status(ataDevice->Channel, ataDevice->Master);
    if (status != ATA_CHK_STATUS_OK)
        return status;

    // Get sector count.
    uint32_t sectorCount = divide_round_up_uint32(length, ataDevice->BytesPerSector);
    if (sectorCount > 256)
        return ATA_STATUS_ERROR;

    // Is the requested sector above 28 bits?
    if (startSectorLba > ATA_MAX_SECTOR_LBA_28BIT) { // 48-bit LBA.
        // Send over high part of sector count.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)((sectorCount >> 8) & 0xFF));

        // Send over high parts of LBA.
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 24) & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 32) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 40) & 0xFF));

        // Send over lower part of sector count.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)(sectorCount & 0xFF));

        // Send over lower parts of LBA and command.
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)(startSectorLba & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 8) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND(ataDevice->Channel->CommandPort), ATA_CMD_READ_SECTOR_EXT);
    }
    else { // 28-bit LBA.
        // If 256 sectors, count is to be zero.
        if (sectorCount == 256)
            sectorCount = 0;

        // Send over sector count, LBA, and command.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)(sectorCount & 0xFF));
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)(startSectorLba & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 8) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 16) & 0xFF));
        outb(ATA_REG_DRIVE_SELECT(ataDevice->Channel->CommandPort), inb(ATA_REG_DRIVE_SELECT(ataDevice->Channel->CommandPort))
            | ATA_DEVICE_FLAGS_LBA | (uint8_t)((startSectorLba >> 24) & 0xF));
        outb(ATA_REG_COMMAND(ataDevice->Channel->CommandPort), ATA_CMD_READ_SECTOR);
    }

    // Wait for device.
    if (ata_wait_for_irq(ataDevice->Channel, ataDevice->Master) != ATA_CHK_STATUS_OK || !ata_wait_for_drq(ataDevice->Channel))
        return ata_check_status(ataDevice->Channel, ataDevice->Master);

    // Read data and return status.
    ata_read_data_pio(ataDevice->Channel, (sectorCount == 0 ? 256 : sectorCount) * ataDevice->BytesPerSector, outData, length);
    return ata_check_status(ataDevice->Channel, ataDevice->Master);
}

int16_t ata_read_dma(ata_device_t *ataDevice, uint64_t startSectorLba, void *outData, uint32_t length) {
    // Check status.
    int16_t status = ata_check_status(ataDevice->Channel, ataDevice->Master);
    if (status != ATA_CHK_STATUS_OK)
        return status;

    // Get sector count.
    uint32_t sectorCount = divide_round_up_uint32(length, ataDevice->BytesPerSector);
    if (sectorCount > 256)
        return ATA_STATUS_ERROR;

    // Reset DMA.
    ata_dma_reset(ataDevice->Channel);

    // Prepare buffers for incoming data.
    uint32_t remainingData = sectorCount * ataDevice->BytesPerSector;
    uint8_t prdIndex = 0;
    while (remainingData > 0) {
        uint32_t size = remainingData;
        if (size > ATA_PRD_BUF_SIZE)
            size = ATA_PRD_BUF_SIZE;

        // Set PRD fields.
        ataDevice->Channel->Prdt[prdIndex].ByteCount = (uint16_t)size;
        ataDevice->Channel->Prdt[prdIndex].EndOfTable = false;

        // Move to next PRD entry.
        prdIndex++;
        remainingData -= size;
    }

    // Last PRD entry needs flag set.
    ataDevice->Channel->Prdt[prdIndex - 1].EndOfTable = true;
    kprintf("ATA: Reading %u sectors now...\n", sectorCount);

    // Is the requested sector above 28 bits?
    if (startSectorLba > ATA_MAX_SECTOR_LBA_28BIT) { // 48-bit LBA.
        // Send over high part of sector count.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)((sectorCount >> 8) & 0xFF));

        // Send over high parts of LBA.
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 24) & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 32) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 40) & 0xFF));

        // Send over lower part of sector count.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)(sectorCount & 0xFF));

        // Send over lower parts of LBA and command.
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)(startSectorLba & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 8) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 16) & 0xFF));
        outb(ATA_REG_COMMAND(ataDevice->Channel->CommandPort), ATA_CMD_READ_DMA_EXT);
    }
    else { // 28-bit LBA.
        // If 256 sectors, count is to be zero.
        if (sectorCount == 256)
            sectorCount = 0;

        // Send over sector count, LBA, and command.
        outb(ATA_REG_SECTOR_COUNT(ataDevice->Channel->CommandPort), (uint8_t)(sectorCount & 0xFF));
        outb(ATA_REG_LBA_LOW(ataDevice->Channel->CommandPort), (uint8_t)(startSectorLba & 0xFF));
        outb(ATA_REG_LBA_MID(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 8) & 0xFF));
        outb(ATA_REG_LBA_HIGH(ataDevice->Channel->CommandPort), (uint8_t)((startSectorLba >> 16) & 0xFF));
        outb(ATA_REG_DRIVE_SELECT(ataDevice->Channel->CommandPort), inb(ATA_REG_DRIVE_SELECT(ataDevice->Channel->CommandPort))
            | ATA_DEVICE_FLAGS_LBA | (uint8_t)((startSectorLba >> 24) & 0xF));
        outb(ATA_REG_COMMAND(ataDevice->Channel->CommandPort), ATA_CMD_READ_DMA);
    }

    // Start transfer with DMA.
    outb(ataDevice->Channel->BusMasterCommandPort, ATA_DMA_CMD_WRITE | ATA_DMA_CMD_START);
    ata_dma_start(ataDevice->Channel, false);

    // Wait for device.
    if (ata_wait_for_irq(ataDevice->Channel, ataDevice->Master) != ATA_CHK_STATUS_OK) {
        // Stop DMA.
        ata_dma_reset(ataDevice->Channel);
        return ata_check_status(ataDevice->Channel, ataDevice->Master);
    }

    // TODO???
    while (inb(ataDevice->Channel->BusMasterStatusPort) & 0x1);

    // Stop DMA.
    ata_dma_reset(ataDevice->Channel);

    remainingData = length;
    prdIndex = 0;
    while (remainingData > 0) {
        uint32_t size = remainingData;
        if (size > ATA_PRD_BUF_SIZE)
            size = ATA_PRD_BUF_SIZE;

        // Copy data.
        memcpy((uint8_t*)outData + (prdIndex * ATA_PRD_BUF_SIZE), ataDevice->Channel->PrdBuffers[prdIndex], size);

        // Move to next PRD entry.
        prdIndex++;
        remainingData -= size;
    }

    return ata_check_status(ataDevice->Channel, ataDevice->Master);
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
