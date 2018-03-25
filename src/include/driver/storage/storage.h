#ifndef STORAGE_H
#define STORAGE_H

#include <main.h>

typedef struct StorageDevice {
    void *Device;

    void (*Read)(struct StorageDevice *storageDevice, uint64_t startByte, uint32_t count, uint8_t *outData);
    void (*Write)(struct StorageDevice *storageDevice, uint64_t startByte, uint32_t count, const uint8_t *data);
} StorageDevice;

#endif
