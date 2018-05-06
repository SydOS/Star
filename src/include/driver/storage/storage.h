#ifndef STORAGE_H
#define STORAGE_H

#include <main.h>

typedef struct storage_device {
    void *Device;

    void (*Read)(struct storage_device *storageDevice, uint64_t startByte, uint32_t count, uint8_t *outData);
    void (*Write)(struct storage_device *storageDevice, uint64_t startByte, uint32_t count, const uint8_t *data);
    uint64_t (*GetSize)(struct storage_device *storageDevice);
} storage_device_t;

extern void storage_register(storage_device_t *device);

#endif
