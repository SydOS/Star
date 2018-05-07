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
    uint8_t BootHeader[3];

    char OemIdentifier[8];

    uint16_t BytesPerSector;

    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t TableCount;
    uint16_t MaxRootDirectoryEntries;
    uint16_t TotalSectors16;
    uint8_t MediaDescriptorType;
    uint16_t TableSize16;
    uint16_t SectorsPerTrack;
    uint16_t HeadsSides;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
} __attribute__((packed)) fat_bpb_header_t;

typedef struct {
    uint8_t DriverNumber;
    uint8_t WindowsNtFlags;
    uint8_t Signature;

    char SerialNumber[4];
    char VolumeLabel[11];
    char SystemIdentifier[8];
} __attribute__((packed)) fat_ebr_header_t;

typedef struct {
    fat_bpb_header_t BiosParameterBlock;
    fat_ebr_header_t ExtendedBootRecord;
} __attribute__((packed)) fat_disk_header_t;

typedef struct {
    uint32_t TotalSectors;
    uint32_t TableSizeSectors;
    

    uint16_t BytesPerSector;

    uint32_t StartTableSector;
    uint32_t StartDataSector;

    uint32_t RootDirectorySizeSectors;
    uint32_t StartRootDirectorySector;
    uint16_t MaxRootDirectoryEntries;

    uint32_t TotalClusters;
    uint32_t TotalDataSectors;

    char VolumeName[12];
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
