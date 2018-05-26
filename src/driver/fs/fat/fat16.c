/*
 * File: fat16.c
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

#include <kernel/storage/storage.h>

void fat16_print_info(fat16_t *fat16Volume) {
    // Null terminate volume label and FS name.
    char tempVolName[12];
    strncpy(tempVolName, fat16Volume->Header.VolumeLabel, 11);
    tempVolName[11] = '\0';
    char fatType[9];
    strncpy(fatType, fat16Volume->Header.FileSystemType, 8);
    fatType[8] = '\0';

    // Print info.
    uint32_t totalSectors = fat16Volume->Header.BPB.TotalSectors == 0 ? fat16Volume->Header.BPB.TotalSectors32 : fat16Volume->Header.BPB.TotalSectors;
    kprintf("FAT: Volume \"%s\" | %u bytes | %u bytes per cluster\n", tempVolName, totalSectors * fat16Volume->Header.BPB.BytesPerSector, fat16Volume->Header.BPB.BytesPerSector * fat16Volume->Header.BPB.SectorsPerCluster);
    kprintf("FAT:   FAT type: %s | Serial number: 0x%X\n", fatType, fat16Volume->Header.SerialNumber);
    kprintf("FAT:   FAT start: sector %u | Length: %u sectors\n", fat16Volume->TableStart, fat16Volume->TableLength);
    kprintf("FAT:   Root Dir start: sector %u | Length: %u sectors\n", fat16Volume->RootDirectoryStart, fat16Volume->RootDirectoryLength);
    kprintf("FAT:   Data start: sector %u | Length: %u sectors\n", fat16Volume->DataStart, fat16Volume->DataLength);
    kprintf("FAT:   Total sectors: %u\n", totalSectors);
}
