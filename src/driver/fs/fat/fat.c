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
#include <kernel/vfs/vfs.h>



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
 * Gets the total number of clusters in a chain.
 */
static inline uint32_t fat_get_num_clusters(fat_t *fat, uint32_t startCluster) {
    // Get clusters.
    uint32_t clusterCount = 0;
    if (fat->Type == FAT_TYPE_FAT12) {
        // Get FAT12 clusters.
        uint16_t cluster = (uint16_t)startCluster;
        while (cluster >= FAT12_CLUSTER_DATA_FIRST && cluster <= FAT12_CLUSTER_DATA_LAST) {
            // Get value of next cluster from FAT.
            cluster = fat12_get_cluster((fat12_cluster_pair_t*)fat->Table, cluster);
            clusterCount++;
        }
    }
    else if (fat->Type == FAT_TYPE_FAT16) {
        // Get FAT16 clusters.
        uint16_t cluster = (uint16_t)startCluster;
        while (cluster >= FAT16_CLUSTER_DATA_FIRST && cluster <= FAT16_CLUSTER_DATA_LAST) {
            // Get value of next cluster from FAT.
            cluster = ((uint16_t*)fat->Table)[cluster];
            clusterCount++;
        }
    }
    else if (fat->Type == FAT_TYPE_FAT32) {
        // Get FAT32 clusters.
        uint32_t cluster = startCluster & FAT32_CLUSTER_MASK;
        while ((cluster & FAT32_CLUSTER_MASK) >= FAT32_CLUSTER_DATA_FIRST && (cluster & FAT32_CLUSTER_MASK) <= FAT32_CLUSTER_DATA_LAST) {
            // Get value of next cluster from FAT.
            cluster = ((uint32_t*)fat->Table)[cluster] & FAT32_CLUSTER_MASK;
            clusterCount++;
        }
    }

    return clusterCount;
}

static inline uint32_t fat_get_total_sectors(fat_t *fatVolume) {
    return fatVolume->BPB->TotalSectors == 0 ? fatVolume->BPB->TotalSectors32 : fatVolume->BPB->TotalSectors;
}

void fat_print_info(fat_t *fatVolume) {
    // Null terminate volume label and FS name.
    char tempVolName[12];
    strncpy(tempVolName, fatVolume->EBPB->VolumeLabel, 11);
    tempVolName[11] = '\0';
    char fatTypeInfo[9];
    strncpy(fatTypeInfo, fatVolume->EBPB->FileSystemType, 8);
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

    // If FAT32, calculate sectors where root directory is.
    uint32_t rootDirLength = fatVolume->RootDirectoryLength;
    uint32_t rootDirStart = fatVolume->RootDirectoryStart;
    if (fatVolume->Type == FAT_TYPE_FAT32) {
        uint32_t clusters = fat_get_num_clusters(fatVolume, fatVolume->RootDirectoryStart);
        rootDirLength = clusters * fatVolume->BPB->SectorsPerCluster;
        rootDirStart = fatVolume->DataStart + (fatVolume->RootDirectoryStart - FAT_CLUSTERS_RESERVED);
    }

    // Print info.
    kprintf("FAT: Volume \"%s\" | %u bytes | %u bytes per cluster\n", tempVolName, fat_get_total_sectors(fatVolume) * fatVolume->BPB->BytesPerSector, fatVolume->BPB->BytesPerSector * fatVolume->BPB->SectorsPerCluster);
    kprintf("FAT:   FAT type: %s (%s) | Serial number: 0x%X\n", fatType, fatTypeInfo, fatVolume->EBPB->SerialNumber);
    kprintf("FAT:   FAT start: sector %u | Length: %u sectors\n", fatVolume->TableStart, fatVolume->TableLength);
    kprintf("FAT:   Root Dir start: sector %u | Length: %u sectors\n", rootDirStart, rootDirLength);
    kprintf("FAT:   Data start: sector %u | Length: %u sectors\n", fatVolume->DataStart, fatVolume->DataLength);
    kprintf("FAT:   Total sectors: %u | Total clusters: %u\n", fat_get_total_sectors(fatVolume), fatVolume->DataClusters);
}

bool fat_clusters_read(fat_t *fat, uint32_t startCluster, uint32_t chainLengthBytes, uint8_t *outBuffer, uint32_t length) {
    // Ensure length isn't bigger than the entry size, if the entry size isn't zero.
    if (chainLengthBytes > 0 && length > chainLengthBytes)
        length = chainLengthBytes;

    // Get the total clusters of the entry.
    uint16_t bytesPerCluster = fat->BPB->SectorsPerCluster * fat->BPB->BytesPerSector;
    uint32_t totalClusters = divide_round_up_uint32(chainLengthBytes > 0 ? chainLengthBytes : length, bytesPerCluster);

    // Create list of clusters.
    uint64_t *blocks = (uint64_t*)kheap_alloc(totalClusters * sizeof(uint64_t));
    memset(blocks, 0, totalClusters);
    uint32_t remainingClusters = totalClusters;
    uint32_t offset = 0;

    // Get clusters.
    if (fat->Type == FAT_TYPE_FAT12) {
        // Get FAT12 clusters.
        uint16_t cluster = (uint16_t)startCluster;
        while (cluster >= FAT12_CLUSTER_DATA_FIRST && cluster <= FAT12_CLUSTER_DATA_LAST && remainingClusters) {
            // Get value of next cluster from FAT.
            uint16_t nextCluster = fat12_get_cluster((fat12_cluster_pair_t*)fat->Table, cluster);

            // Add cluster to block list.
            blocks[offset] = fat->DataStart + ((cluster - FAT_CLUSTERS_RESERVED)) * fat->BPB->SectorsPerCluster;

            offset++;
            cluster = nextCluster;
            remainingClusters--;
        }
    }
    else if (fat->Type == FAT_TYPE_FAT16) {
        // Get FAT16 clusters.
        uint16_t cluster = (uint16_t)startCluster;
        while (cluster >= FAT16_CLUSTER_DATA_FIRST && cluster <= FAT16_CLUSTER_DATA_LAST && remainingClusters) {
            // Get value of next cluster from FAT.
            uint16_t nextCluster = ((uint16_t*)fat->Table)[cluster];

            // Add cluster to block list.
            blocks[offset] = fat->DataStart + ((cluster - FAT_CLUSTERS_RESERVED)) * fat->BPB->SectorsPerCluster;

            offset++;
            cluster = nextCluster;
            remainingClusters--;
        }
    }
    else if (fat->Type == FAT_TYPE_FAT32) {
        // Get FAT32 clusters.
        uint32_t cluster = startCluster & FAT32_CLUSTER_MASK;
        while ((cluster & FAT32_CLUSTER_MASK) >= FAT32_CLUSTER_DATA_FIRST && (cluster & FAT32_CLUSTER_MASK) <= FAT32_CLUSTER_DATA_LAST) {
            // Get value of next cluster from FAT.
            uint32_t nextCluster = ((uint32_t*)fat->Table)[cluster] & FAT32_CLUSTER_MASK;

            // Add cluster to block list.
            blocks[offset] = fat->DataStart + ((cluster - FAT_CLUSTERS_RESERVED)) * fat->BPB->SectorsPerCluster;

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
    bool result = fat->Device->ReadBlocks(fat->Device, fat->PartitionIndex, blocks, fat->BPB->SectorsPerCluster, totalClusters, outBuffer, length);

    // Free cluster list.
    kheap_free(blocks);
    return result;
}

bool fat_get_dir(fat_t *fat, uint32_t cluster, fat_dir_entry_t **outDirEntries, uint32_t *outEntryCount) {
    // FAT12 and FAT16 have a static root directory, with others in data area.
    if (cluster == 0 && (fat->Type == FAT_TYPE_FAT12 || fat->Type == FAT_TYPE_FAT16)) {
        // Allocate space for directory bytes.
        const uint32_t rootDirLength = sizeof(fat_dir_entry_t) * fat->BPB->MaxRootDirectoryEntries;
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
    else { // FAT32 has all directories in data area.
        // Determine size of root directory.
        uint32_t dirLength = fat_get_num_clusters(fat, cluster) * fat->BPB->BytesPerSector * fat->BPB->SectorsPerCluster;
        fat_dir_entry_t *dirEntries = (fat_dir_entry_t*)kheap_alloc(dirLength);
        memset(dirEntries, 0, dirLength);

        // Read root directory.
        if (!fat_clusters_read(fat, cluster, dirLength, dirEntries, dirLength)) {
            kheap_free(dirEntries);
            return false;
        }

        // Count up entries.
        uint32_t entryCount = 0;
        for (uint32_t i = 0; i < dirLength / sizeof(fat_dir_entry_t) && dirEntries[i].FileName[0] != 0; i++)
            entryCount++;

        // Reduce size of directory array.
        dirEntries = (fat_dir_entry_t*)kheap_realloc(dirEntries, entryCount * sizeof(fat_dir_entry_t));

        // Get outputs.
        *outDirEntries = dirEntries;
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

            // Determine cluster.
            uint32_t cluster = directoryEntries[i].StartClusterLow;
            if (fat->Type == FAT_TYPE_FAT32)
                cluster += directoryEntries[i].StartClusterHigh << 16;

            fat_clusters_read(fat, cluster, directoryEntries[i].Length, bees, directoryEntries[i].Length);
            bees[directoryEntries[i].Length] ='\0';
            kprintf(bees);
            kheap_free(bees);
        }
    }
}

static bool fat_verify_lfn_checksum(const char *shortName, uint8_t checksum) {
    uint8_t sum = 0;
    for (uint32_t i = 11; i; i--)
        sum = ((sum & 1) << 7) + (sum >> 1) + *shortName++;

    return sum == checksum;
}

bool fat_vfs_get_dir_nodes(vfs_node_t *fsNode, vfs_node_t **outDirNodes, uint32_t *outCount) {
    // Get FAT volume and directory entry.
    fat_t *fatVolume = (fat_t*)fsNode->FsObject;
    fat_dir_entry_t *fatDirEntry = (fat_dir_entry_t*)fsNode->FsFileObject;

    // Variables to hold directory entries.
    fat_dir_entry_t *fatDirEntries;
    uint32_t fatDirEntriesCount = 0;

    // If the starting cluster is zero, its the root directory.
    uint32_t cluster = fatDirEntry->StartClusterLow + (fatDirEntry->StartClusterHigh >> 16);
    fat_get_dir(fatVolume, cluster, &fatDirEntries, &fatDirEntriesCount);
        

    vfs_node_t *dirNodes = (vfs_node_t*)kheap_alloc(sizeof(vfs_node_t) * fatDirEntriesCount);
    memset(dirNodes, 0, sizeof(vfs_node_t) * fatDirEntriesCount);

    // Generate VFS nodes.
    uint32_t destIndex = 0;
    fat_lfn_segment_t *lfnFirstSegment = NULL;
    fat_lfn_segment_t *lfnCurrentSegment = NULL;
    for (uint32_t i = 0; i < fatDirEntriesCount; i++) {
        // If the attributes are 0x0F, its an LFN entry.
        if (fatDirEntries[i].VolumeLabel && fatDirEntries[i].System && fatDirEntries[i].Hidden && fatDirEntries[i].ReadOnly) {
            // Create next segment.
            lfnCurrentSegment = (fat_lfn_segment_t*)kheap_alloc(sizeof(fat_lfn_segment_t));
            memset(lfnCurrentSegment, 0, sizeof(fat_lfn_segment_t));
            lfnCurrentSegment->NextSegment = lfnFirstSegment;
            lfnFirstSegment = lfnCurrentSegment;
               
            // Get LFN entry.
            fat_dir_lfn_entry_t *lfnEntry = (fat_dir_lfn_entry_t*)(fatDirEntries + i);
            lfnCurrentSegment->SequenceNumber = lfnEntry->SequenceNumber;
            lfnCurrentSegment->Checksum = lfnEntry->Checksum;

            // Get name characters.
            for (uint32_t i = 0; i < 5; i++)
                lfnCurrentSegment->Name[i] = (char)lfnEntry->Name1[i];
            for (uint32_t i = 0; i < 6; i++)
                lfnCurrentSegment->Name[i+5] = (char)lfnEntry->Name2[i];
            for (uint32_t i = 0; i < 2; i++)
                lfnCurrentSegment->Name[i+5+6] = (char)lfnEntry->Name3[i];

            // Move onto next entry.
            continue;
        }

        // Do we have LFN segments present? If so use that name instead of the 8.3 name.
        if (lfnFirstSegment != NULL && fat_verify_lfn_checksum(fatDirEntries[i].FileName, lfnFirstSegment->Checksum)) {
            // Count segments.
            uint32_t lfnSegmentCount = 0;
            lfnCurrentSegment = lfnFirstSegment;
            while (lfnSegmentCount <= 20 && lfnCurrentSegment != NULL) {
                lfnSegmentCount++;
                lfnCurrentSegment = lfnCurrentSegment->NextSegment;
            }

            // Allocate string.
            dirNodes[destIndex].Name = (char*)kheap_alloc(lfnSegmentCount * FAT_LFN_FILENAME_LENGTH);
            lfnCurrentSegment = lfnFirstSegment;
            for (uint32_t i = 0; i < lfnSegmentCount; i++) {
                strncpy(dirNodes[destIndex].Name + (i * FAT_LFN_FILENAME_LENGTH), lfnCurrentSegment->Name, FAT_LFN_FILENAME_LENGTH);
                lfnCurrentSegment = lfnCurrentSegment->NextSegment;
            }

            // Reduce allocation size if needed.
            uint32_t nameLength = strlen(dirNodes[destIndex].Name);
            if (nameLength + 1 < lfnSegmentCount * FAT_LFN_FILENAME_LENGTH)
                dirNodes[destIndex].Name = (char*)kheap_realloc(dirNodes[destIndex].Name, nameLength + 1);
            dirNodes[destIndex].Name[nameLength] = '\0';
        }
        else {
            // Trim spaces from filename.
            uint32_t nameLength = FAT_FILENAME_LENGTH;
            char *nameStrPtr = fatDirEntries[i].FileName + nameLength;
            while (isspace((char)*(--nameStrPtr)) && nameLength > 0) {
                nameLength--;
            }

            // Trim spaces from extension
            uint32_t extLength = FAT_EXT_LENGTH;
            char *extStrPtr = fatDirEntries[i].FileName + FAT_FILENAME_LENGTH + extLength;
            while (isspace((char)*(--extStrPtr)) && extLength > 0) {
                extLength--;
            }

            // Allocate space for name, dot, extension, and null terminator.
            uint32_t totalLength = nameLength + 1 + (extLength > 0 ? extLength + 1 : 0);
            dirNodes[destIndex].Name = (char*)kheap_alloc(totalLength);
            strncpy(dirNodes[destIndex].Name, fatDirEntries[i].FileName, nameLength);

            // Get extension if there is one.
            if (extLength > 0) {
                strncpy(dirNodes[destIndex].Name + nameLength + 1, fatDirEntries[i].FileName + 8, extLength);
                dirNodes[destIndex].Name[nameLength] = '.';
            }
            dirNodes[destIndex].Name[totalLength - 1] = '\0';
        }

        // Set other objects.
        dirNodes[destIndex].FsObject = fatVolume;
        dirNodes[destIndex].FsFileObject = fatDirEntries+i;
        dirNodes[destIndex].GetDirNodes = fat_vfs_get_dir_nodes;
        destIndex++;
    }

    // Resize as needed.
    dirNodes = (vfs_node_t*)kheap_realloc(dirNodes, sizeof(vfs_node_t) * destIndex);

    *outDirNodes = dirNodes;
    *outCount = destIndex;
    return true;
}

fat_t *fat_init(storage_device_t *storageDevice, uint16_t partitionIndex) {
    kprintf("FAT: Init\n");

    // Get first sector of drive, which contains the FAT header.
    uint8_t *fatHeader = (uint8_t*)kheap_alloc(FAT_HEADER_SIZE_MAX);
    kprintf("FAT: first\n");
    if (!storageDevice->ReadSectors(storageDevice, partitionIndex, FAT_HEADER_SECTOR, fatHeader, FAT_HEADER_SIZE_MAX)) {
        kheap_free(fatHeader);
        return NULL;
    }

    // Create FAT object.
    fat_t *fatVolume = (fat_t*)kheap_alloc(sizeof(fat_t));
    memset(fatVolume, 0, sizeof(fat_t));
    fatVolume->Device = storageDevice;
    fatVolume->PartitionIndex = partitionIndex;
    fatVolume->Header = fatHeader;
    fatVolume->BPB = (fat_bpb_header_t*)fatVolume->Header;

    // Determine type of FAT.
    if ((partitionIndex != PARTITION_NONE) && (storageDevice->PartitionMap->Partitions[partitionIndex]->FsType == FILESYSTEM_TYPE_FAT32)) {
        // FAT is FAT32.
        fatVolume->Type = FAT_TYPE_FAT32;

        // Header has BPB, EBPB, and the FAT32 EBPB.
        fatVolume->EBPB32 = (fat32_ebpb_header_t*)(fatVolume->Header + sizeof(fat_bpb_header_t)); 
        fatVolume->EBPB = (fat_ebpb_header_t*)(fatVolume->Header + sizeof(fat_bpb_header_t) + sizeof(fat32_ebpb_header_t)); 
    }
    else {
        // Determine type based on cluster count.
        if (fatVolume->DataClusters > FAT12_CLUSTERS_MAX)
            fatVolume->Type = FAT_TYPE_FAT16;
        else
            fatVolume->Type = FAT_TYPE_FAT12;

        // Header has BPB and EBPB.
        fatVolume->EBPB32 = NULL;
        fatVolume->EBPB = (fat_ebpb_header_t*)(fatVolume->Header + sizeof(fat_bpb_header_t));
    }

    // Determine location and size of FAT.
    fatVolume->TableStart = fatVolume->BPB->ReservedSectorsCount;
    if (fatVolume->Type == FAT_TYPE_FAT32)
        fatVolume->TableLength = fatVolume->BPB->TableSize == 0 ? fatVolume->EBPB32->TableSize32 : fatVolume->BPB->TableSize;
    else
        fatVolume->TableLength = fatVolume->BPB->TableSize;

    // Can't have zero length FAT.
    if (fatVolume->TableLength == 0)
        panic("FAT: Zero length FAT!\n"); // TODO don't panic.

    // FAT32 does not have a static root directory, but FAT12/16 does.
    if (fatVolume->Type == FAT_TYPE_FAT32) {
        // Root directory is stored in clusters, so save the first one.
        fatVolume->RootDirectoryStart = fatVolume->EBPB32->RootDirectoryCluster;
        fatVolume->RootDirectoryLength = 0; // Unused.

        // Data area is directly after the FAT in FAT32.
        fatVolume->DataStart = fatVolume->TableStart + (fatVolume->TableLength * fatVolume->BPB->TableCount);
        fatVolume->DataLength = fat_get_total_sectors(fatVolume) - fatVolume->DataStart;
        fatVolume->DataClusters = fatVolume->DataLength / fatVolume->BPB->SectorsPerCluster;
    }
    else {
        // Root directory is located after the FAT in FAT12/16.
        fatVolume->RootDirectoryStart = fatVolume->TableStart + (fatVolume->TableLength * fatVolume->BPB->TableCount);
        fatVolume->RootDirectoryLength = ((fatVolume->BPB->MaxRootDirectoryEntries * sizeof(fat_dir_entry_t)) + (fatVolume->BPB->BytesPerSector - 1)) / fatVolume->BPB->BytesPerSector;

        // Data area is after the root directory.
        fatVolume->DataStart = fatVolume->RootDirectoryStart + fatVolume->RootDirectoryLength;
        fatVolume->DataLength = fat_get_total_sectors(fatVolume) - fatVolume->DataStart;
        fatVolume->DataClusters = fatVolume->DataLength / fatVolume->BPB->SectorsPerCluster;
    }

    // Get File Allocation Table.
    uint32_t fatClustersLength = fatVolume->TableLength * fatVolume->BPB->BytesPerSector;
    fatVolume->Table = (uint8_t*)kheap_alloc(fatClustersLength);
    memset(fatVolume->Table, 0, fatClustersLength);
    storageDevice->ReadSectors(storageDevice, partitionIndex, fatVolume->TableStart, fatVolume->Table, fatClustersLength);
    
    // Populate root directory.
    fatVolume->RootDirectory = (fat_dir_entry_t*)kheap_alloc(sizeof(fat_dir_entry_t));
    memset(fatVolume->RootDirectory, 0, sizeof(fat_dir_entry_t));
    fatVolume->RootDirectory->FileName[0] = '/';
    fatVolume->RootDirectory->Subdirectory = true;
    fatVolume->RootDirectory->StartClusterLow = 0;
    fatVolume->RootDirectory->StartClusterHigh = 0;
    fatVolume->RootDirectory->Length = 0;
    
    // Print info.
    fat_print_info(fatVolume);

    // Get root dir.
   /* fat_dir_entry_t *rootDirEntries;
    uint32_t rootDirEntriesCount = 0;
    fat_get_root_dir(fatVolume, &rootDirEntries, &rootDirEntriesCount);
    fat_print_dir(fatVolume, rootDirEntries, rootDirEntriesCount, 0);
    kheap_free(rootDirEntries);*/

    // Return FAT object.
    return fatVolume;

    // Free stuff for now.
   // kheap_free(fatVolume->Table);
   // kheap_free(fatVolume->Header);
   // kheap_free(fatVolume);
}
