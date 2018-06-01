/*
 * File: partition_map.c
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
#include <io.h>
#include <kprint.h>
#include <kernel/storage/storage.h>
#include <kernel/storage/partition_map.h>

void part_print_map(partition_map_t *partMap) {
    switch (partMap->Type) {
        case PARTITION_MAP_TYPE_MBR:
            kprintf("PARTMAP: Master Boot Record\n");
            break;

        case PARTITION_MAP_TYPE_GPT:
            kprintf("PARTMAP: Master Boot Record\n");
            break;

        default:
            kprintf("PARTMAP: Unknown\n");
            break;
    }
    
    for (uint8_t i = 0; i < partMap->NumPartitions; i++) {
        char *fsType = "Unknown";
        switch (partMap->Partitions[i]->FsType) {
            case FILESYSTEM_TYPE_FAT:
                fsType = "FAT12/FAT16";
                break;

            case FILESYSTEM_TYPE_FAT32:
                fsType = "FAT32";
                break;
        }
        kprintf("PARTMAP:   PART%u: Start: %u | End: %u | Type: %s\n", i + 1, partMap->Partitions[i]->LbaStart, partMap->Partitions[i]->LbaEnd, fsType);
    }
}
