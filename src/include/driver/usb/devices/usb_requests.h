/*
 * File: usb_requests.h
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

#ifndef USB_REQUESTS_H
#define USB_REQUESTS_H

#include <main.h>

// USB request bits.
#define USB_REQUEST_TYPE_STANDARD       0x0
#define USB_REQUEST_TYPE_CLASS          0x1
#define USB_REQUEST_TYPE_VENDOR         0x2

#define USB_REQUEST_REC_DEVICE          0x0
#define USB_REQUEST_REC_INTERFACE       0x1
#define USB_REQUEST_REC_ENDPOINT        0x2
#define USB_REQUEST_REC_OTHER           0x3

// Standard USB request.
#define USB_REQUEST_GET_STATUS      0
#define USB_REQUEST_CLEAR_FEATURE   1
#define USB_REQUEST_SET_FEATURE     3
#define USB_REQUEST_SET_ADDRESS     5
#define USB_REQUEST_GET_DESCRIPTOR  6
#define USB_REQUEST_SET_DESCRIPTOR  7
#define USB_REQUEST_GET_CONFIG      8
#define USB_REQUEST_SET_CONFIG      9
#define USB_REQUEST_GET_INTERFACE   10
#define USB_REQUEST_SET_INTERFACE   11
#define USB_REQUEST_SYNCH_FRAME     12

// Standard USB feature selectors.
#define USB_REQUEST_FEAT_DEVICE_REMOTE_WAKEUP   1
#define USB_REQUEST_FEAT_ENDPOINT_HALT          0
#define USB_REQUEST_FEAT_TEST_MODE              2

// USB request structure.
typedef struct {
    // Characteristics of request.
    uint8_t Recipient : 5;
    uint8_t Type : 2;
    bool Inbound : 1;

    // Specific request.
    uint8_t Request;

    // Word-sized field that varies according to request.
    uint8_t ValueLow;
    uint8_t ValueHigh;

    // Word-sized field that varies according to request;
    // typically used to pass an index or offset.
    uint16_t Index;

    // Number of bytes to transfer if there is a Data stage.
    uint16_t Length;
} __attribute__((packed)) usb_request_t;

#endif
