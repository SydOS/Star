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

static inline uint32_t fat_get_total_sectors(fat_t *fatVolume) {
    return fatVolume->Header->BPB.TotalSectors == 0 ? fatVolume->Header->BPB.TotalSectors32 : fatVolume->Header->BPB.TotalSectors;
}

void fat_print_info(fat_t *fatVolume) {
    // Null terminate volume label and FS name.
    char tempVolName[12];
    strncpy(tempVolName, fatVolume->Header->VolumeLabel, 11);
    tempVolName[11] = '\0';
    char fatTypeInfo[9];
    strncpy(fatTypeInfo, fatVolume->Header->FileSystemType, 8);
    fatTypeInfo[8] = '\0';

    // Determine type.
    char *fatType = "Unknown";
    switch (fatVolume->Type) {
        case FAT_TYPE_FAT12:
            fatType = "FAT12";
            break;

        case FAT_TYPE_FAT16:
            fatType = "FAT16";
            break;

        case FAT_TYPE_FAT32:
            fatType = "FAT32";
            break;
    }

    // Print info.
    kprintf("FAT: Volume \"%s\" | %u bytes | %u bytes per cluster\n", tempVolName, fat_get_total_sectors(fatVolume) * fatVolume->Header->BPB.BytesPerSector, fatVolume->Header->BPB.BytesPerSector * fatVolume->Header->BPB.SectorsPerCluster);
    kprintf("FAT:   FAT type: %s (%s) | Serial number: 0x%X\n", fatType, fatTypeInfo, fatVolume->Header->SerialNumber);
    kprintf("FAT:   FAT start: sector %u | Length: %u sectors\n", fatVolume->TableStart, fatVolume->TableLength);
    kprintf("FAT:   Root Dir start: sector %u | Length: %u sectors\n", fatVolume->RootDirectoryStart, fatVolume->RootDirectoryLength);
    kprintf("FAT:   Data start: sector %u | Length: %u sectors\n", fatVolume->DataStart, fatVolume->DataLength);
    kprintf("FAT:   Total sectors: %u | Total clusters: %u\n", fat_get_total_sectors(fatVolume), fatVolume->DataClusters);
}

/**
 * Gets the next FAT12 cluster in the specified chain.
 * @clusters    The array of clusters to pull from.
 * @clusterNum  The index of the cluster.
 */
static inline uint16_t fat12_get_cluster(fat12_cluster_pair_t *clusters, uint16_t clusterNum) {
    // Get low or high part of 24-bit number.
    if (clusterNum % 2)
        return clusters[clusterNum / 2].Cluster2;
    else 
        return clusters[clusterNum / 2].Cluster1;
}

bool fat_entry_read(fat_t *fat, fat_dir_entry_t *entry, uint8_t *outBuffer, uint32_t length) {
    // Ensure length isn't bigger than the entry size, if the entry size isn't zero.
    if (entry->Length > 0 && length > entry->Length)
        length = entry->Length;

    // Get the total clusters of the entry.
    uint16_t bytesPerCluster = fat->Header->BPB.SectorsPerCluster * fat->Header->BPB.BytesPerSector;
    uint32_t totalClusters = DIVIDE_ROUND_UP(entry->Length > 0 ? entry->Length : length, bytesPerCluster);

    // Create list of clusters.
    uint64_t *blocks = (uint64_t*)kheap_alloc(totalClusters * sizeof(uint64_t));
    memset(blocks, 0, totalClusters);
    uint32_t remainingClusters = totalClusters;
    uint32_t offset = 0;

    // Get clusters.
    if (fat->Type == FAT_TYPE_FAT12) {
        // Get FAT12 clusters.
        uint16_t cluster = entry->StartClusterLow;
        while (cluster >= FAT12_CLUSTER_DATA_FIRST && cluster <= FAT12_CLUSTER_DATA_LAST && remainingClusters) {
            // Get value of next cluster from FAT.
            uint16_t nextCluster = fat12_get_cluster((fat12_cluster_pair_t*)fat->Table, cluster);

            // Add cluster to block list.
            blocks[offset] = fat->DataStart + ((cluster - FAT12_CLISTERS_RESERVED)) * fat->Header->BPB.SectorsPerCluster;

            offset++;
            cluster = nextCluster;
            remainingClusters--;
        }
    }
    else if (fat->Type == FAT_TYPE_FAT16) {
        // Get FAT16 clusters.
        uint16_t cluster = entry->StartClusterLow;
        while (cluster >= FAT12_CLUSTER_DATA_FIRST && cluster <= FAT12_CLUSTER_DATA_LAST && remainingClusters) {
            // Get value of next cluster from FAT.
            uint16_t nextCluster = ((uint16_t*)fat->Table)[cluster];

            // Add cluster to block list.
            blocks[offset] = fat->DataStart + ((cluster - FAT16_CLISTERS_RESERVED)) * fat->Header->BPB.SectorsPerCluster;

            offset++;
            cluster = nextCluster;
            remainingClusters--;
        }
    }
    else {
        // Unknown FAT type.
        kheap_free(blocks);
        return false;
    }

    // Read blocks from storage device.
    bool result = fat->Device->ReadBlocks(fat->Device, fat->PartitionIndex, blocks, fat->Header->BPB.SectorsPerCluster, totalClusters, outBuffer, length);

    // Free cluster list.
    kheap_free(blocks);
    return result;
}

bool fat_get_root_dir(fat_t *fat, fat_dir_entry_t **outDirEntries, uint32_t *outEntryCount) {
    // FAT12 and FAT16 have a static root directory.
    if (fat->Type == FAT_TYPE_FAT12 || fat->Type == FAT_TYPE_FAT16) {
        // Allocate space for directory bytes.
        const uint32_t rootDirLength = sizeof(fat_dir_entry_t) * fat->Header->BPB.MaxRootDirectoryEntries;
        fat_dir_entry_t *rootDirEntries = (fat_dir_entry_t*)kheap_alloc(rootDirLength);
        memset(rootDirEntries, 0, rootDirLength);

        // Read from storage.
        bool result = fat->Device->ReadSectors(fat->Device, fat->PartitionIndex, fat->RootDirectoryStart, rootDirEntries, rootDirLength);
        if (!result) {
            kheap_free(rootDirEntries);
            return false;
        }

        // Count up entries.
        uint32_t entryCount = 0;
        for (uint32_t i = 0; i < rootDirLength / sizeof(fat_dir_entry_t) && rootDirEntries[i].FileName[0] != 0; i++)
            entryCount++;

        // Reduce size of directory array.
        rootDirEntries = (fat_dir_entry_t*)kheap_realloc(rootDirEntries, entryCount * sizeof(fat_dir_entry_t));

        // Get outputs.
        *outDirEntries = rootDirEntries;
        *outEntryCount = entryCount;
        return true;
    }

    // Unknown FAT type.
    return false;
}

void fat_print_dir(fat_t *fat, fat_dir_entry_t *directoryEntries, uint32_t directoryEntriesCount, uint32_t level) {
    for (uint32_t i = 0; i < directoryEntriesCount; i++) {
        kprintf("FAT:  ");
        for (uint32_t p = 0; p < level; p++)
            kprintf(" ");
        

        // Get file name and extension.
        char fileName[9];
        strncpy(fileName, directoryEntries[i].FileName, 8);
        fileName[8] = '\0';

        char extension[4];
        strncpy(extension, directoryEntries[i].FileName+8, 3);
        extension[3] = '\0';

        if (extension[0] != ' ' && extension[0] != '\0')
            kprintf("%s: %s.%s (%u bytes)\n", directoryEntries[i].Subdirectory ? "DIR " : "FILE", fileName, extension, directoryEntries[i].Length);
        else
            kprintf("%s: %s (%u bytes)\n", directoryEntries[i].Subdirectory ? "DIR " : "FILE", fileName, directoryEntries[i].Length);

       /* if (directoryEntries[i].Subdirectory && directoryEntries[i].FileName[0] != '.') {
            fat_dir_entry_t *subEntries;
            uint32_t subCount = 0;
            fat_get_dir_fat12(fat, directoryEntries+i, &subEntries, &subCount);
            fat_print_dir(fat, subEntries, subCount, level+1);
            kheap_free(subEntries);
        }*/

        if (strcmp(fileName, "BEEMOVIE") == 0) {
            uint8_t *bees = (uint8_t*)kheap_alloc(directoryEntries[i].Length + 1);
            fat_entry_read(fat, directoryEntries+i, bees, directoryEntries[i].Length);
            bees[directoryEntries[i].Length] ='\0';
            kprintf(bees);
            kheap_free(bees);
        }
    }
}

bool fat_init(storage_device_t *storageDevice, uint16_t partitionIndex) {
    // Get header in first sector.
    fat_header_t *fatHeader = (fat_header_t*)kheap_alloc(sizeof(fat_header_t));
    memset(fatHeader, 0, sizeof(fat_header_t));
    storageDevice->ReadSectors(storageDevice, partitionIndex, 0, fatHeader, sizeof(fat_header_t));

    // Get first sector of FAT.
    uint64_t fatVersion = 0;
    storageDevice->ReadSectors(storageDevice, partitionIndex, fatHeader->BPB.ReservedSectorsCount, &fatVersion, sizeof(fatVersion));
    uint8_t fatDriveType = fatVersion & 0xFF;

    // Create FAT object.
    fat_t *fatVolume = (fat_t*)kheap_alloc(sizeof(fat_t));
    memset(fatVolume, 0, sizeof(fat_t));

    // Populate fields.
    fatVolume->Device = storageDevice;
    fatVolume->PartitionIndex = partitionIndex;
    fatVolume->Header = fatHeader;
    fatVolume->TableStart = fatVolume->Header->BPB.ReservedSectorsCount;
    fatVolume->TableLength = fatVolume->Header->BPB.TableSize;
    fatVolume->RootDirectoryStart = fatVolume->TableStart + (fatVolume->Header->BPB.TableSize * fatVolume->Header->BPB.TableCount);
    fatVolume->RootDirectoryLength = ((fatVolume->Header->BPB.MaxRootDirectoryEntries * sizeof(fat_dir_entry_t)) + (fatVolume->Header->BPB.BytesPerSector - 1)) / fatVolume->Header->BPB.BytesPerSector;
    fatVolume->DataStart = fatVolume->RootDirectoryStart + fatVolume->RootDirectoryLength;
    uint32_t totalSectors = fatVolume->Header->BPB.TotalSectors == 0 ? fatVolume->Header->BPB.TotalSectors32 : fatVolume->Header->BPB.TotalSectors;
    fatVolume->DataLength = totalSectors - fatVolume->DataStart;
    fatVolume->DataClusters = fatVolume->DataLength / fatVolume->Header->BPB.SectorsPerCluster;

    // Determine type based on cluster count.
    if (fatVolume->DataClusters > FAT12_CLUSTERS_MAX)
        fatVolume->Type = FAT_TYPE_FAT16;
    else
        fatVolume->Type = FAT_TYPE_FAT12;

    // Get File Allocation Table.
    uint32_t fatClustersLength = fatVolume->TableLength * fatVolume->Header->BPB.BytesPerSector;
    fatVolume->Table = (uint8_t*)kheap_alloc(fatClustersLength);
    memset(fatVolume->Table, 0, fatClustersLength);
    storageDevice->ReadSectors(storageDevice, partitionIndex, fatVolume->TableStart, fatVolume->Table, fatClustersLength);
    
    // Print info.
    fat_print_info(fatVolume);

    // Get root dir.
    fat_dir_entry_t *rootDirEntries;
    uint32_t rootDirEntriesCount = 0;
    fat_get_root_dir(fatVolume, &rootDirEntries, &rootDirEntriesCount);
    fat_print_dir(fatVolume, rootDirEntries, rootDirEntriesCount, 0);
    kheap_free(rootDirEntries);

    // Free stuff for now.
    kheap_free(fatVolume->Table);
    kheap_free(fatVolume->Header);
    kheap_free(fatVolume);
}
