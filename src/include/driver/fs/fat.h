/*
 * File: fat.h
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

#ifndef FS_FAT_H
#define FS_FAT_H

#include <main.h>
#include <kernel/storage/storage.h>
#include <kernel/vfs/vfs.h>

#define FAT_DIRECTORY_ENTRY_SIZE    32

#define FAT_VERSION_MASK_FAT12      0xFFFFF0
#define FAT_VERSION_MASK_FAT16      0xFFFFFFF0

#define FAT12_CLUSTERS_MAX          4084
#define FAT16_CLUSTERS_MAX          65524
#define FAT32_CLUSTERS_MAX          268435444
#define FAT_CLUSTERS_RESERVED       2

// FAT12 clusters.
#define FAT12_CLUSTER_FREE          0x000
#define FAT12_CLUSTER_RESERVED      0x001
#define FAT12_CLUSTER_DATA_FIRST    0x002
#define FAT12_CLUSTER_DATA_LAST     0xFEF
#define FAT12_CLUSTER_BAD           0xFF7
#define FAT12_CLUSTER_EOC1          0xFF8
#define FAT12_CLUSTER_EOC2          0xFF0

// FAT16 clusters.
#define FAT16_CLUSTER_FREE          0x0000
#define FAT16_CLUSTER_RESERVED      0x0001
#define FAT16_CLUSTER_DATA_FIRST    0x0002
#define FAT16_CLUSTER_DATA_LAST     0xFFEF
#define FAT16_CLUSTER_BAD           0xFFF7
#define FAT16_CLUSTER_EOC           0xFFF8

// FAT32 clusters.
#define FAT32_CLUSTER_FREE          0x0000000
#define FAT32_CLUSTER_RESERVED      0x0000001
#define FAT32_CLUSTER_DATA_FIRST    0x0000002
#define FAT32_CLUSTER_DATA_LAST     0xFFFFFEF
#define FAT32_CLUSTER_BAD           0xFFFFFF7
#define FAT32_CLUSTER_EOC           0xFFFFFF8
#define FAT32_CLUSTER_MASK          0x0FFFFFFF

#define FAT_TYPE_FAT12  1
#define FAT_TYPE_FAT16  2
#define FAT_TYPE_FAT32  3

#define FAT_FILENAME_LENGTH     8
#define FAT_EXT_LENGTH          3
#define FAT_LFN_FILENAME_LENGTH 13

typedef struct {
    // Jump instruction.
    uint8_t JumpInstruction[3];

    // OEM name. Determines what system formatted the disk.
    char OemName[8];

    // Bytes per sector, typically 512.
    uint16_t BytesPerSector;

    // Number of sectors per cluster, used for determining cluster size.
    uint8_t SectorsPerCluster;

    // Number of reserved sectors, which is the number of sectors
    // before the first FAT on disk.
    uint16_t ReservedSectorsCount;

    // Number of File Allocation Tables (FATs). Usually 2.
    uint8_t TableCount;

    // Maximum amount of root directory entries in FAT12/FAT16.
    // This is zero for FAT32.
    uint16_t MaxRootDirectoryEntries;

    // Total number of sectors if volume is under 32MB. FAT32 may set this value to zero.
    // If zero, use the 32-bit count value.
    uint16_t TotalSectors;

    // Media descriptor.
    uint8_t MediaType;

    // Size of File Allocation Tables (FATs) in sectors.
    // FAT32 sets this to zero.
    uint16_t TableSize;

    // Number of physical sectors per track on drives that use CHS.
    uint16_t SectorsPerTrack;

    // Number of heads for disks that use CHS.
    uint16_t HeadCount;

    // Number of hidden sectors preceding this FAT partition, or zero for unpartitioned disks.
    uint32_t HiddenSectors;

    // Total number of sectors if it exceeds 65535 (32MB).
    uint32_t TotalSectors32;
} __attribute__((packed)) fat_bpb_header_t;

// Extended BIOS Parameter Block.
typedef struct {
    // Physical drive number.
    uint8_t DriveNumber;

    // Reserved, generally used by Windows NT.
    uint8_t Reserved;

    // Extended boot signature. Should be either 0x29 or 0x28.
    uint8_t ExtendedBootSignature;

    // Volume serial number.
    uint32_t SerialNumber;

    // Volume label.
    char VolumeLabel[11];

    // Type of FAT, should be used for display only.
    char FileSystemType[8];
} __attribute__((packed)) fat_ebpb_header_t;

// FAT32 Extended BIOS Parameter Block.
typedef struct {
    // Logical sectors per file allocation table.
    uint32_t TableSize32;

    // Drive description.
    uint16_t DriveDescription;

    // Version.
    uint16_t Version;

    // Root directory cluster.
    uint32_t RootDirectoryCluster;

    // Sector where the FS info sector resides.
    uint16_t FsInfoSector;

    uint16_t FirstBootSector;

    // Reserved.
    uint8_t Reserved[12];
} __attribute__((packed)) fat32_ebpb_header_t;

#define FAT_HEADER_SECTOR       0
#define FAT_HEADER_SIZE_MAX     (sizeof(fat_bpb_header_t) + sizeof(fat_ebpb_header_t) + sizeof(fat32_ebpb_header_t))

typedef struct {
    uint16_t Cluster1 : 12;
    uint16_t Cluster2 : 12;
} __attribute__((packed)) fat12_cluster_pair_t;

// FAT directory entry.
typedef struct {
    char FileName[11];

    // Attributes.
    bool ReadOnly : 1;
    bool Hidden : 1;
    bool System : 1;
    bool VolumeLabel : 1;
    bool Subdirectory : 1;
    bool Archive : 1;
    bool Device : 1;
    bool Reserved : 1;

    uint8_t WindowsNtReserved;

    uint8_t CreationTimeTenthsSecond;
    uint16_t CreationTime;
    uint16_t CreationDate;
    uint16_t LastAccessedDate;

    uint16_t StartClusterHigh;

    uint16_t LastModificationTime;
    uint16_t LastModificationDate;

    uint16_t StartClusterLow;
    uint32_t Length;
} __attribute__((packed)) fat_dir_entry_t;

// FAT LFN entry.
typedef struct {
    uint8_t SequenceNumber;
    uint16_t Name1[5];
    uint8_t Attributes;
    uint8_t Type;
    uint8_t Checksum;
    uint16_t Name2[6];
    uint16_t FirstCluster;
    uint16_t Name3[2];
} __attribute__((packed)) fat_dir_lfn_entry_t;

// FAT.
typedef struct {
    // Underlying storage device.
    storage_device_t *Device;
    uint16_t PartitionIndex;

    // Header area.
    uint8_t *Header;

    // Structs that point to header above.
    fat_bpb_header_t *BPB;
    fat_ebpb_header_t *EBPB;
    fat32_ebpb_header_t *EBPB32;

    // Type of FAT.
    uint8_t Type;

    // FAT starting sector and length in sectors.
    uint32_t TableStart;
    uint32_t TableLength;

    // FATs.
    uint8_t *Table;
    uint8_t *TableSpare;

     // Root directory starting sector and length in sectors.
    uint32_t RootDirectoryStart;
    uint32_t RootDirectoryLength;

    // Root directory.
    fat_dir_entry_t *RootDirectory;

    // Data area starting sector and length in sectors.
    uint32_t DataStart;
    uint32_t DataLength;
    uint32_t DataClusters;
} fat_t;

// FAT LFN chain entries.
typedef struct {
    uint8_t SequenceNumber;
    uint8_t Checksum;
    char Name[13];
    struct fat_lfn_segment_t *NextSegment;
} fat_lfn_segment_t;

// FAT12.

extern bool fat_vfs_get_dir_nodes(vfs_node_t *fsNode, vfs_node_t **outDirNodes, uint32_t *outCount);
extern fat_t *fat_init(storage_device_t *storageDevice, uint16_t partitionIndex);


#endif
