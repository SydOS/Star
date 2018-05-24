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
#include <driver/fs/fat.h>
#include <kernel/memory/kheap.h>

#include <math.h>

#include <driver/storage/floppy.h>
#include <driver/storage/storage.h>



bool fat_init(storage_device_t *storageDevice) {
    // Get header.
    fat12_header_t *fatHeader = (fat12_header_t*)kheap_alloc(sizeof(fat12_header_t));
    memset(fatHeader, 0, sizeof(fat12_header_t));

    storageDevice->Read(storageDevice, 0, fatHeader, sizeof(fat12_header_t));

    // Create FAT object.
    fat12_t *fatVolume = (fat12_t*)kheap_alloc(sizeof(fat12_t));
    memset(fatVolume, 0, sizeof(fat12_t));

    // Populate.
    fatVolume->Device = storageDevice;
    fatVolume->Header = *fatHeader;
    fatVolume->TableStart = fatVolume->Header.BPB.ReservedSectorsCount;
    fatVolume->TableLength = fatVolume->Header.BPB.TableSize;
    fatVolume->RootDirectoryStart = fatVolume->TableStart + (fatVolume->Header.BPB.TableSize * fatVolume->Header.BPB.TableCount);
    fatVolume->RootDirectoryLength = ((fatVolume->Header.BPB.MaxRootDirectoryEntries * sizeof(fat_dir_entry_t)) + (fatVolume->Header.BPB.BytesPerSector - 1)) / fatVolume->Header.BPB.BytesPerSector;
    fatVolume->DataStart = fatVolume->RootDirectoryStart + fatVolume->RootDirectoryLength;
    fatVolume->DataLength = fatVolume->Header.BPB.TotalSectors - fatVolume->DataStart;
    kheap_free(fatHeader);

    char tempVolName[12];
    strncpy(tempVolName, fatVolume->Header.VolumeLabel, 11);
    tempVolName[11] = '\0';

    char fatType[9];
    strncpy(fatType, fatVolume->Header.FileSystemType, 8);
    fatType[8] = '\0';

    // Get total sectors.
    kprintf("FAT: Volume \"%s\" | %u bytes | %u sectors per cluster\n", tempVolName, fatVolume->Header.BPB.TotalSectors * fatVolume->Header.BPB.BytesPerSector, fatVolume->Header.BPB.SectorsPerCluster);
    kprintf("FAT:   FAT type: %s | Serial number: 0x%X\n", fatType, fatVolume->Header.SerialNumber);
    kprintf("FAT:   FAT start: sector %u | Length: %u sectors\n", fatVolume->TableStart, fatVolume->TableLength);
    kprintf("FAT:   Root Dir start: sector %u | Length: %u sectors\n", fatVolume->RootDirectoryStart, fatVolume->RootDirectoryLength);
    kprintf("FAT:   Data start: sector %u | Length: %u sectors\n", fatVolume->DataStart, fatVolume->DataLength);
    kprintf("FAT:   Total sectors: %u\n", fatVolume->Header.BPB.TotalSectors);

    // Get the FAT.
    uint32_t fat12ClustersLength = fatVolume->TableLength * fatVolume->Header.BPB.BytesPerSector;
    fatVolume->Table = (fat12_cluster_pair_t*)kheap_alloc(fat12ClustersLength);
    memset(fatVolume->Table, 0, fat12ClustersLength);
    storageDevice->Read(storageDevice, fatVolume->TableStart * fatVolume->Header.BPB.BytesPerSector, fatVolume->Table, fat12ClustersLength);

    // Get root dir.
    fat_dir_entry_t *rootDirEntries;
    uint32_t rootDirEntriesCount = 0;
    fat12_get_root_dir(fatVolume, &rootDirEntries, &rootDirEntriesCount);
    fat12_print_dir(fatVolume, rootDirEntries, rootDirEntriesCount, 0);
    kheap_free(rootDirEntries);
    kheap_free(fatVolume->Table);
    kheap_free(fatVolume);
}
