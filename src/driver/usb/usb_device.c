#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>

#include <kernel/memory/kheap.h>

bool usb_device_send_request(usb_device_t *device, usb_request_t *request, void *data) {
    // Create request for USB device.

    // Send request to USB device and return status.
}

usb_device_t *usb_device_create() {
    // Allocate space for a USB device and return it.
    usb_device_t *device = (usb_device_t*)kheap_alloc(sizeof(usb_device_t));
    memset(device, 0, sizeof(usb_device_t));
    return device;
}

bool usb_device_init(usb_device_t *device) {

}