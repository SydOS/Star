/*
 * File: mbr.c
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
#include <kprint.h>
#include <driver/storage/mbr.h>
#include <kernel/storage/storage.h>
#include <kernel/memory/kheap.h>

#include <driver/fs/fat.h>

void mbr_print(mbr_t *mbr) {
    kprintf("MBR: Disk signature: 0x%X\n", mbr->Signature1);
    for (uint8_t i = 0; i < MBR_NO_OF_PARTITIONS; i++) {
        kprintf("MBR:   PART%u: Start: %u | End: %u | Type: 0x%X\n", i + 1, mbr->Entries[i].StartLba, mbr->Entries[i].StartLba + mbr->Entries[i].CountLba, mbr->Entries[i].Type);
    }
}

bool mbr_init(storage_device_t *storageDevice) {
    // Get MBR.
    mbr_t *mbr = (mbr_t*)kheap_alloc(sizeof(mbr_t));
    storageDevice->ReadSectors(storageDevice, PARTITION_NONE, 0, mbr, sizeof(mbr_t));

    // Get number of partitions.
    uint16_t numPartitions = 0;
    for (uint8_t i = 0; i < MBR_NO_OF_PARTITIONS; i++)
        if (mbr->Entries[i].Type)
            numPartitions++;

    // Create parition map.
    const size_t mapSize = sizeof(partition_map_t) + (sizeof(partition_t) * (numPartitions - 1));
    storageDevice->PartitionMap = (partition_map_t*)kheap_alloc(sizeof(partition_map_t));
    memset(storageDevice->PartitionMap, 0, sizeof(partition_map_t));
    storageDevice->PartitionMap->Type = PARTITION_MAP_TYPE_MBR;

    // Populate partitions.
    storageDevice->PartitionMap->NumPartitions = 0;
    uint32_t p = 0;
    for (uint16_t i = 0; i < MBR_NO_OF_PARTITIONS; i++) {
        if (!mbr->Entries[i].Type)
            continue;

        // Allocate space for partition.
        storageDevice->PartitionMap->NumPartitions++;
        storageDevice->PartitionMap->Partitions = (partition_t**)kheap_realloc(storageDevice->PartitionMap->Partitions, storageDevice->PartitionMap->NumPartitions * sizeof(partition_t*));
        storageDevice->PartitionMap->Partitions[p] = (partition_t*)kheap_alloc(sizeof(partition_t));

        // Determine LBA.
        uint64_t lbaStart = mbr->Entries[i].StartLba;
        uint64_t lbaEnd = mbr->Entries[i].StartLba + mbr->Entries[i].CountLba;

        storageDevice->PartitionMap->Partitions[p]->LbaStart = lbaStart;
        storageDevice->PartitionMap->Partitions[p]->LbaEnd = lbaEnd;

        // Determine type.
        if (mbr->Entries[i].Type == MBR_TYPE_FAT12_L32MB || mbr->Entries[i].Type == MBR_TYPE_FAT16 || mbr->Entries[i].Type == MBR_TYPE_FAT16B || mbr->Entries[i].Type == MBR_TYPE_FAT16_LBA)
            storageDevice->PartitionMap->Partitions[p]->FsType = FILESYSTEM_TYPE_FAT;

        // Move to next partition.
        p++;
    }  

    // Print out map.
    part_print_map(storageDevice->PartitionMap);

    // Test FAT16.
    if (storageDevice->PartitionMap->Partitions[0]->FsType == FILESYSTEM_TYPE_FAT) {
        fat_init(storageDevice, 0);
    }
}