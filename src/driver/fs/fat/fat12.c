/*
 * File: fat12.c
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
#include <driver/fs/fat.h>
#include <kernel/memory/kheap.h>

#include <math.h>

#include <driver/storage/floppy.h>
#include <kernel/storage/storage.h>

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

/**
 * Gets the total number of FAT12 clusters.
 */
static inline uint32_t fat12_get_num_clusters(fat12_t *fat, fat_dir_entry_t *entry) {
    // Get clusters.
    uint16_t cluster = entry->StartClusterLow;
    uint32_t clusterCount = 0;
    while (cluster >= 0x002 && cluster <= 0xFEF) {
        // Get value of next cluster from FAT.
        cluster = fat12_get_cluster(fat->Table, cluster);
        clusterCount++;
    }

    return clusterCount;
}

bool fat12_entry_read(fat12_t *fat, fat_dir_entry_t *entry, uint8_t *outBuffer, uint32_t length) {
    // Ensure length isn't bigger than the entry size, if the entry size isn't zero.
    if (entry->Length > 0 && length > entry->Length)
        length = entry->Length;

    // Get the total clusters of the entry.
    uint16_t bytesPerCluster = fat->Header.BPB.SectorsPerCluster * fat->Header.BPB.BytesPerSector;
    uint32_t totalClusters = DIVIDE_ROUND_UP(entry->Length > 0 ? entry->Length : length, bytesPerCluster);

    // Create list of clusters.
    uint64_t *blocks = (uint64_t*)kheap_alloc(totalClusters * sizeof(uint64_t));
    memset(blocks, 0, totalClusters);
    uint32_t remainingClusters = totalClusters;
    uint32_t offset = 0;

    // Get clusters.
    uint16_t cluster = entry->StartClusterLow;
    while (cluster >= 0x002 && cluster <= 0xFEF && remainingClusters) {
        // Get value of next cluster from FAT.
        uint16_t nextCluster = fat12_get_cluster(fat->Table, cluster);

        // Add cluster to block list.
        blocks[offset] = (fat->DataStart + cluster - 2) * bytesPerCluster;

        offset++;
        cluster = nextCluster;
        remainingClusters--;
    }

    // Read blocks from storage device.
    bool result = fat->Device->ReadBlocks(fat->Device, blocks, 1, totalClusters, outBuffer, length);

    // Free cluster list.
    kheap_free(blocks);
    return result;
}

void fat_display_entries(fat_dir_entry_t *dirEntries, uint32_t entryCount) {
    //for (uint32_t i = 0; i < entryCount)
        //kprintf("FAT:  %s: %s");
}

bool fat12_get_dir(fat12_t *fat, fat_dir_entry_t *directory, fat_dir_entry_t **outDirEntries, uint32_t *outEntryCount) {
    // Get number of clusters directory consumes.
    uint32_t clusterCount = fat12_get_num_clusters(fat, directory);

    // Determine number of bytes in clusters.
    uint16_t bytesPerCluster = fat->Header.BPB.SectorsPerCluster * fat->Header.BPB.BytesPerSector;
    uint32_t length = clusterCount * bytesPerCluster;

    // Allocate space for directory bytes.
    fat_dir_entry_t *directoryEntries = (fat_dir_entry_t*)kheap_alloc(length);
    memset(directoryEntries, 0, length);

    // Read from storage.
    bool result = fat12_entry_read(fat, directory, (uint8_t*)directoryEntries, length);
    if (!result) {
        kheap_free(directoryEntries);
        return false;
    }
    
    // Count up entries.
    uint32_t entryCount = 0;
    for (uint32_t i = 0; i < length / sizeof(fat_dir_entry_t) && directoryEntries[i].FileName[0] != 0; i++)
        entryCount++;

    // Reduce size of directory array.
    directoryEntries = (fat_dir_entry_t*)kheap_realloc(directoryEntries, entryCount * sizeof(fat_dir_entry_t));

    // Get outputs.
    *outDirEntries = directoryEntries;
    *outEntryCount = entryCount;
    return true;
}

bool fat12_get_root_dir(fat12_t *fat, fat_dir_entry_t **outDirEntries, uint32_t *outEntryCount) {
    // Allocate space for directory bytes.
    const uint32_t rootDirLength = sizeof(fat_dir_entry_t) * fat->Header.BPB.MaxRootDirectoryEntries;
    fat_dir_entry_t *rootDirEntries = (fat_dir_entry_t*)kheap_alloc(rootDirLength);
    memset(rootDirEntries, 0, rootDirLength);

    // Read from storage.
    bool result = fat->Device->Read(fat->Device, fat->RootDirectoryStart * fat->Header.BPB.BytesPerSector, rootDirEntries, rootDirLength);
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

void fat12_print_dir(fat12_t *fat, fat_dir_entry_t *directoryEntries, uint32_t directoryEntriesCount, uint32_t level) {
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
            fat12_entry_read(fat, directoryEntries+i, bees, directoryEntries[i].Length);
            bees[directoryEntries[i].Length] ='\0';
            kprintf(bees);
            kheap_free(bees);
        }
    }
}
