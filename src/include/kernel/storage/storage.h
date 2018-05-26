/*
 * File: storage.h
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

#ifndef STORAGE_H
#define STORAGE_H

#include <main.h>
#include <kernel/storage/partition_map.h>

typedef struct storage_device_t {
    struct storage_device_t *Next;
    struct storage_device_t *Prev;

    void *Device;
    partition_map_t *PartitionMap;
    

    bool (*Read)(struct storage_device_t *storageDevice, uint64_t startByte, uint8_t *outBuffer, uint32_t length);
    bool (*ReadSectors)(struct storage_device_t *storageDevice, uint16_t partitionIndex, uint64_t startSector, uint32_t sectorCount, uint8_t *outBuffer, uint32_t length);
    void (*Write)(struct storage_device_t *storageDevice, uint64_t startByte, uint32_t count, const uint8_t *data);
    uint64_t (*GetSize)(struct storage_device_t *storageDevice);

    bool (*ReadBlocks)(struct storage_device_t *storageDevice, uint64_t *blocks, uint32_t blockSize, uint32_t blockCount, uint8_t *outBuffer, uint32_t length);
} storage_device_t;

extern storage_device_t *storageDevices;
extern void storage_register(storage_device_t *device);

#endif
