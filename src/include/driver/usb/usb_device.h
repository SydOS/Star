#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <main.h>

// USB speeds.
#define USB_SPEED_FULL      0x0
#define USB_SPEED_LOW       0x1
#define USB_SPEED_HIGH      0x2

#define USB_MAX_DEVICES     127

// Classes
#define USB_CLASS_DEVICE            0x00
#define USB_CLASS_AUDIO             0x01
#define USB_CLASS_COMM_CDC          0x02
#define USB_CLASS_HID               0x03
#define USB_CLASS_PHYSICAL          0x05
#define USB_CLASS_IMAGING           0x06
#define USB_CLASS_PRINTER           0x07
#define USB_CLASS_MASS_STORAGE      0x08
#define USB_CLASS_HUB               0x09
#define USB_CLASS_CDC_DATA          0x0A
#define USB_CLASS_SMART_CARD        0x0B
#define USB_CLASS_CONTENT_SECURITY  0x0D
#define USB_CLASS_VIDEO             0x0E
#define USB_CLASS_PERSONAL_HEALTH   0x0F
#define USB_CLASS_AUDIO_VIDEO       0x10
#define USB_CLASS_BILLBOARD         0x11
#define USB_CLASS_TYPE_C_BRIDGE     0x12
#define USB_CLASS_DIAGNOSTIC        0xDC
#define USB_CLASS_WIRELESS_CTRL     0xE0
#define USB_CLASS_MISC              0xEF
#define USB_CLASS_APP_SPECIFIC      0xFE
#define USB_CLASS_VEN_SPECIFIC      0xFF

typedef struct usb_device_t {
    // Relationship to other USB devices.
    struct usb_device_t *Parent;
    struct usb_device_t *Next;

    // Pointer to controller that device is on.
    void *Controller;

    bool (*AllocAddress)(struct usb_device_t* device);
    void (*FreeAddress)(struct usb_device_t* device);   
    bool (*ControlTransfer)(struct usb_device_t* device, uint8_t endpoint, bool inbound, uint8_t type,
        uint8_t recipient, uint8_t requestType, uint8_t valueLo, uint8_t valueHi, uint16_t index, void *buffer, uint16_t length);

    // Port device is on.
    uint8_t Port;
    uint8_t Speed;
    uint8_t Address;

    // Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid).
    uint8_t MaxPacketSize;

    bool Configured;
    uint8_t ConfigurationValue;
} usb_device_t;

extern usb_device_t *usb_device_create();
extern bool usb_device_init(usb_device_t *device);
extern bool usb_device_configure(usb_device_t *usbDevice);

#endif
