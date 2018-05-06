/*
 * File: usb_hid.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
