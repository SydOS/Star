#ifndef USB_HID_H
#define USB_HID_H

#include <main.h>

#define USB_HID_SUBCLASS_BOOT       0x1
#define USB_HID_PROTOCOL_NONE       0x0
#define USB_HID_PROTOCOL_KEYBOARD   0x1
#define USB_HID_PROTOCOL_MOUSE      0x2

#define USB_DESCRIPTOR_TYPE_HID         0x21
#define USB_DESCRIPTOR_TYPE_REPORT      0x22
#define USB_DESCRIPTOR_TYPE_PHYSICAL    0x23

#define USB_HID_REQUEST_SET_REPORT      0x09

// USB HID descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    uint16_t BcdHid;

    uint8_t CountryCode;

    uint8_t NumDescriptors;

    uint8_t DescriptorType;

    uint16_t DescriptorLength;
} __attribute__((packed)) usb_descriptor_hid_t;

#endif
