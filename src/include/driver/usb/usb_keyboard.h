#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <main.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_hid.h>

typedef struct {
    bool LeftControl : 1;
    bool LeftShift : 1;
    bool LeftAlt : 1;
    bool LeftGui : 1;
    bool RightControl : 1;
    bool RightShift : 1;
    bool RightAlt : 1;
    bool RightGui : 1;

    uint8_t Reserved;
    uint8_t Keycode1;
    uint8_t Keycode2;
    uint8_t Keycode3;
    uint8_t Keycode4;
    uint8_t Keycode5;
    uint8_t Keycode6;
} __attribute__((packed)) usb_keyboard_input_report_t;

typedef struct {
    usb_device_t *Device;
    usb_descriptor_hid_t *Descriptor;

    uint8_t InputEndpointAddress;
} usb_keyboard_t;

#endif
