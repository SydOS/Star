#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

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

bool usb_device_init(usb_device_t *usbDevice) {
    // Create descriptor object.
    usb_descriptor_device_t *desc = (usb_descriptor_device_t*)kheap_alloc(sizeof(usb_descriptor_device_t)); // (usb_descriptor_device_t*)((uintptr_t)req + PAGE_SIZE_4K);
    memset(desc, 0, sizeof(usb_descriptor_device_t));

    // Get first 8 bytes of device descriptor. This will tell us the max packet size.
    usbDevice->ControlTransfer(usbDevice, 0, true, 0, 0, USB_REQUEST_GET_DESCRIPTOR, 0, 0x1, 0, desc, 8);
    usbDevice->MaxPacketSize = desc->MaxPacketSize;

}