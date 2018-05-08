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
#include <driver/storage/storage.h>

#define FAT_DIRECTORY_ENTRY_SIZE    32

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

    // Total number of sectors. FAT32 may set this value to zero.
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

    // Total number of sectors if it exceeds 65535.
    uint32_t TotalSectors32;
} __attribute__((packed)) fat_bpb_header_t;

typedef struct {
    // BIOS Parameter Block.
    fat_bpb_header_t BPB;
    
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
} __attribute__((packed)) fat_header_t;

typedef struct {
    uint16_t Cluster1 : 12;
    uint16_t Cluster2 : 12;
} __attribute__((packed)) fat12_cluster_pair_t;

typedef struct {
    // Header area.
    fat_header_t Header;

    // FAT starting sector and length in sectors.
    uint32_t TableStart;
    uint32_t TableLength;

    fat12_cluster_pair_t *Table;

    // Root directory starting sector and length in sectors.
    uint32_t RootDirectoryStart;
    uint32_t RootDirectoryLength;

    // Data area starting sector and length in sectors.
    uint32_t DataStart;
    uint32_t DataLength;
} fat_t;

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
    uint32_t FileSize;
} __attribute__((packed)) fat_directory_entry_t;

extern bool fat_init(storage_device_t *storageDevice);

#endif
