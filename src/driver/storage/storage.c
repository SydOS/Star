#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/storage/storage.h>

#include <kernel/memory/kheap.h>

// Storage device array.
static storage_device_t **storageDevices = NULL;
static uint32_t storageDevicesLength = 0;

void storage_register(storage_device_t *device) {
    // Add space to array.
    storageDevices = kheap_realloc(storageDevices, storageDevicesLength * sizeof(uintptr_t));

    // Add device pointer to end.
    storageDevices[storageDevicesLength - 1] = device;
}