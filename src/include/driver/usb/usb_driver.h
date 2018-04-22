#ifndef USB_DRIVER_H
#define USB_DRIVER_H

#include <main.h>
#include <driver/usb/usb_device.h>

// Driver object.
typedef struct {
    char *Name;

    bool (*Initialize)(usb_device_t *usbDevice, usb_interface_t *interface, uint8_t *interfaceConfBuffer, uint8_t *interfaceConfBufferEnd);
} usb_driver_t;

// Driver array.
extern const usb_driver_t UsbDrivers[];

#endif
