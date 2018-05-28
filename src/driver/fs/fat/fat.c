/*
 * File: fat.c
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
#include <string.h>
#include <driver/fs/fat.h>
#include <kernel/memory/kheap.h>

#include <math.h>

#include <driver/storage/floppy.h>
#include <kernel/storage/storage.h>



bool fat_init(storage_device_t *storageDevice, uint16_t partitionIndex) {
    // Get header in first sector.
    fat_header_t *fatHeader = (fat_header_t*)kheap_alloc(sizeof(fat_header_t));
    memset(fatHeader, 0, sizeof(fat_header_t));
    storageDevice->ReadSectors(storageDevice, partitionIndex, 0, fatHeader, sizeof(fat_header_t));

    // Get first sector of FAT.
    uint64_t fatVersion = 0;
    storageDevice->ReadSectors(storageDevice, partitionIndex, fatHeader->BPB.ReservedSectorsCount, &fatVersion, sizeof(fatVersion));
    uint8_t fatDriveType = fatVersion & 0xFF;

    // Determine signature.
    if ((fatVersion & FAT_VERSION_MASK_FAT16) == FAT_VERSION_MASK_FAT16) {
        // FAT16.
        fat16_t *fat16Volume = (fat16_t*)kheap_alloc(sizeof(fat16_t));
        memset(fat16Volume, 0, sizeof(fat16_t));

        // Populate fields.
        fat16Volume->Device = storageDevice;
        fat16Volume->PartitionIndex = partitionIndex;
        fat16Volume->Header = *fatHeader;
        fat16Volume->TableStart = fat16Volume->Header.BPB.ReservedSectorsCount;
        fat16Volume->TableLength = fat16Volume->Header.BPB.TableSize;
        fat16Volume->RootDirectoryStart = fat16Volume->TableStart + (fat16Volume->Header.BPB.TableSize * fat16Volume->Header.BPB.TableCount);
        fat16Volume->RootDirectoryLength = ((fat16Volume->Header.BPB.MaxRootDirectoryEntries * sizeof(fat_dir_entry_t)) + (fat16Volume->Header.BPB.BytesPerSector - 1)) / fat16Volume->Header.BPB.BytesPerSector;
        fat16Volume->DataStart = fat16Volume->RootDirectoryStart + fat16Volume->RootDirectoryLength;
        uint32_t totalSectors = fat16Volume->Header.BPB.TotalSectors == 0 ? fat16Volume->Header.BPB.TotalSectors32 : fat16Volume->Header.BPB.TotalSectors;
        fat16Volume->DataLength = totalSectors - fat16Volume->DataStart;

        // Print info.
        fat16_print_info(fat16Volume);

        // Get the FATs.
        uint32_t fat16ClustersLength = fat16Volume->TableLength * fat16Volume->Header.BPB.BytesPerSector;
        fat16Volume->Table = (uint16_t*)kheap_alloc(fat16ClustersLength);
        memset(fat16Volume->Table, 0, fat16ClustersLength);
        storageDevice->ReadSectors(storageDevice, partitionIndex, fat16Volume->TableStart, fat16Volume->Table, 1024);//fat16ClustersLength);

        // Get root dir.
        fat_dir_entry_t *rootDirEntries;
        uint32_t rootDirEntriesCount = 0;
        fat16_get_root_dir(fat16Volume, &rootDirEntries, &rootDirEntriesCount);
        fat16_print_dir(fat16Volume, rootDirEntries, rootDirEntriesCount, 0);
        kheap_free(rootDirEntries);
        kheap_free(fat16Volume->Table);
        kheap_free(fat16Volume);
    }
    else if ((fatVersion & FAT_VERSION_MASK_FAT12) == FAT_VERSION_MASK_FAT12) {
        // FAT12.
        fat12_t *fat12Volume = (fat12_t*)kheap_alloc(sizeof(fat12_t));
        memset(fat12Volume, 0, sizeof(fat12_t));

        // Populate fields.
        fat12Volume->Device = storageDevice;
        fat12Volume->PartitionIndex = partitionIndex;
        fat12Volume->Header = *fatHeader;
        fat12Volume->TableStart = fat12Volume->Header.BPB.ReservedSectorsCount;
        fat12Volume->TableLength = fat12Volume->Header.BPB.TableSize;
        fat12Volume->RootDirectoryStart = fat12Volume->TableStart + (fat12Volume->Header.BPB.TableSize * fat12Volume->Header.BPB.TableCount);
        fat12Volume->RootDirectoryLength = ((fat12Volume->Header.BPB.MaxRootDirectoryEntries * sizeof(fat_dir_entry_t)) + (fat12Volume->Header.BPB.BytesPerSector - 1)) / fat12Volume->Header.BPB.BytesPerSector;
        fat12Volume->DataStart = fat12Volume->RootDirectoryStart + fat12Volume->RootDirectoryLength;
        fat12Volume->DataLength = fat12Volume->Header.BPB.TotalSectors - fat12Volume->DataStart;

        // Print info.
        fat12_print_info(fat12Volume);

        // Get the FATs.
        uint32_t fat12ClustersLength = fat12Volume->TableLength * fat12Volume->Header.BPB.BytesPerSector;
        fat12Volume->Table = (fat12_cluster_pair_t*)kheap_alloc(fat12ClustersLength);
        memset(fat12Volume->Table, 0, fat12ClustersLength);
        storageDevice->ReadSectors(storageDevice, partitionIndex, fat12Volume->TableStart, fat12Volume->Table, fat12ClustersLength);

        // Get root dir.
        fat_dir_entry_t *rootDirEntries;
        uint32_t rootDirEntriesCount = 0;
        fat12_get_root_dir(fat12Volume, &rootDirEntries, &rootDirEntriesCount);
        fat12_print_dir(fat12Volume, rootDirEntries, rootDirEntriesCount, 0);
        kheap_free(rootDirEntries);
        kheap_free(fat12Volume->Table);
        kheap_free(fat12Volume);
    }
}
