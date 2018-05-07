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

#include <driver/storage/floppy.h>
#include <driver/storage/storage.h>

bool fat_init(storage_device_t *storageDevice) {
    // Get header.
    fat_disk_header_t *fatHeader = (fat_disk_header_t*)kheap_alloc(sizeof(fat_disk_header_t));
    memset(fatHeader, 0, sizeof(fat_disk_header_t));

    storageDevice->Read(storageDevice, 0, sizeof(fat_disk_header_t), fatHeader);

    // Create FAT object.
    fat_t *fatVolume = (fat_t*)kheap_alloc(sizeof(fat_t));
    memset(fatVolume, 0, sizeof(fat_t));

    // Populate.
    fatVolume->MaxRootDirectoryEntries = fatHeader->BiosParameterBlock.MaxRootDirectoryEntries;
    fatVolume->TotalSectors = (fatHeader->BiosParameterBlock.TotalSectors16 == 0) ? fatHeader->BiosParameterBlock.TotalSectors32 : fatHeader->BiosParameterBlock.TotalSectors16;
    fatVolume->TableSizeSectors = fatHeader->BiosParameterBlock.TableSize16;
    fatVolume->RootDirectorySizeSectors = ((fatVolume->MaxRootDirectoryEntries * FAT_DIRECTORY_ENTRY_SIZE) + (fatHeader->BiosParameterBlock.BytesPerSector - 1)) / fatHeader->BiosParameterBlock.BytesPerSector;
    fatVolume->StartTableSector = fatHeader->BiosParameterBlock.ReservedSectors;
    fatVolume->StartDataSector = fatVolume->StartTableSector + (fatHeader->BiosParameterBlock.TableCount * fatVolume->TableSizeSectors) + fatVolume->RootDirectorySizeSectors;
    fatVolume->BytesPerSector = fatHeader->BiosParameterBlock.BytesPerSector;
    fatVolume->StartRootDirectorySector = fatVolume->StartDataSector - fatVolume->RootDirectorySizeSectors;

    fatVolume->TotalDataSectors = fatVolume->TotalSectors - (fatHeader->BiosParameterBlock.ReservedSectors + (fatHeader->BiosParameterBlock.TableCount * fatVolume->TableSizeSectors) + fatVolume->RootDirectorySizeSectors);
    fatVolume->TotalClusters = fatVolume->TotalDataSectors / fatHeader->BiosParameterBlock.SectorsPerCluster;

    strncpy(fatVolume->VolumeName, fatHeader->ExtendedBootRecord.VolumeLabel, 11);
    fatVolume->VolumeName[11] = '\n';

    // Get total sectors.
    kprintf("FAT: Total sectors: %u\n", fatVolume->TotalSectors);
    kprintf("FAT: Total size in bytes: %u\n", fatVolume->TotalSectors * fatVolume->BytesPerSector);
    kprintf("FAT: Label: %s\n", fatVolume->VolumeName);

    // Get root directory.
    fat_directory_entry_t *rootDir = (fat_directory_entry_t*)kheap_alloc(sizeof(fat_directory_entry_t) * fatVolume->MaxRootDirectoryEntries);
    memset(rootDir, 0, sizeof(fat_directory_entry_t) * fatVolume->MaxRootDirectoryEntries);
char dd[12];
    //floppy_read_sector(0, 18, rootDir, 32);
    storageDevice->Read(storageDevice, fatVolume->StartRootDirectorySector * fatVolume->BytesPerSector, sizeof(fat_directory_entry_t) * fatVolume->MaxRootDirectoryEntries, rootDir);
    for (uint8_t i = 0; i < fatVolume->MaxRootDirectoryEntries && rootDir[i].FileName[0] != 0; i++) {
        if (rootDir[i].FileName[0] == 0xE5)
            continue;

        strncpy(dd, rootDir[i].FileName, 11);
        dd[11] = '\0';
        kprintf("FAT: %u: %s: %s | Size: %u bytes | Cluster %u\n", i, rootDir[i].Subdirectory ? "Folder" : "File", dd, rootDir[i].FileSize, rootDir[i].StartClusterLow);
    }
}
