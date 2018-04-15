#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <main.h>

// USB speeds.
#define USB_SPEED_FULL      0x0
#define USB_SPEED_LOW       0x1
#define USB_SPEED_HIGH      0x2

typedef struct usb_device_t {
    // Relationship to other USB devices.
    struct usb_device_t *Parent;
    struct usb_device_t *Next;

    // Pointer to controller that device is on.
    void *Controller;
    
    void (*Control)(struct usb_device_t* device, uint8_t endpoint);

    // Port device is on.
    uint8_t Port;
    uint8_t Speed;
    uint8_t Address;

    // Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid).
    uint8_t MaxPacketSize;
} usb_device_t;

extern usb_device_t *usb_device_create();
extern bool usb_device_init(usb_device_t *device);

#endif
