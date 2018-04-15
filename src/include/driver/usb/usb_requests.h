#ifndef USB_REQUESTS_H
#define USB_REQUESTS_H

#include <main.h>

// Standard USB request types.
#define USB_REQUEST_TYPE_DEVICE_TO_HOST 0x80
#define USB_REQUEST_TYPE_HOST_TO_DEVICE 0x00

#define USB_REQUEST_TYPE_VENDOR         0x40
#define USB_REQUEST_TYPE_CLASS          0x20
#define USB_REQUEST_TYPE_STANDARD       0x00

#define USB_REQUEST_TYPE_REC_MASK       0x1F
#define USB_REQUEST_TYPE_REC_OTHER      0x03
#define USB_REQUEST_TYPE_REC_ENDPOINT   0x02
#define USB_REQUEST_TYPE_REC_INTERFACE  0x01
#define USB_REQUEST_TYPE_REC_DEVICE     0x00

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

// Hub USB requests.
#define USB_REQUEST_HUB_CLEAR_TT_BUFFER 8
#define USB_REQUEST_HUB_RESET_TT        9
#define USB_REQUEST_HUB_GET_TT_STATE    10
#define USB_REQUEST_STOP_TT             11

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
