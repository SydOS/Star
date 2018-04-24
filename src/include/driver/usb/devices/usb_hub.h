#ifndef USB_HUB_H
#define USB_HUB_H

#include <main.h>
#include <driver/usb/devices/usb_descriptors.h>
#include <driver/usb/devices/usb_device.h>

#define USB_HUB_POWERSW_GANGED      0x0
#define USB_HUB_POWERSW_INDIV       0x1

#define USB_HUB_OVERCURRENT_GLOBAL  0x0
#define USB_HUB_OVERCURRENT_INDIV   0x1

#define USB_HUB_POWERTOPORT_UNITS   2

// Hub USB requests.
#define USB_REQUEST_HUB_CLEAR_TT_BUFFER 8
#define USB_REQUEST_HUB_RESET_TT        9
#define USB_REQUEST_HUB_GET_TT_STATE    10
#define USB_REQUEST_STOP_TT             11

// Hub class features.
#define USB_HUB_FEAT_PORT_RESET         4
#define USB_HUB_FEAT_PORT_POWER         8

// USB hub port status.
typedef struct {
    bool Connected : 1;
    bool Enabled : 1;
    bool Suspended : 1;
    bool Overcurrent : 1;
    bool Reset : 1;
    uint8_t Reserved1 : 3;

    bool Powered : 1;
    bool LowSpeed : 1;
    bool HighSpeed : 1;
    bool TestMode : 1;
    bool PortIndicatorSoftwareControlled : 1;
    uint8_t Reserved2 : 3;

    bool LocalPowerStatusChanged : 1;
    bool OvercurrentChanged : 1;
    uint16_t Reserved3 : 14;
} __attribute__((packed)) usb_hub_port_status_t;

// USB hub descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // Number of downstream ports.
    uint8_t NumPorts;

    // Characteristics.
    uint8_t PowerSwitchingMode : 2;
    bool CompoundDevice : 1;
    uint8_t OvercurrentMode : 2;
    uint8_t TtThinkTime : 2;
    bool PortIndicatorsSupported : 1;
    uint8_t Reserved1 : 8;

    // Time (in 2 ms intervals) from the time the power-on sequence
    // begins on a port until power is good on that port.
    uint8_t PowerOnToPowerGood;

    // Maximum current requirements of the Hub Controller electronics in mA.
    uint8_t HubControllerCurrent;

    // Removable bitmap and power control masks are variable.
} __attribute__((packed)) usb_descriptor_hub_t;

typedef struct {
    usb_device_t *Device;
    usb_descriptor_hub_t *Descriptor;

    usb_endpoint_t *StatusEndpoint;
} usb_hub_t;

extern bool usb_hub_init(usb_device_t *usbDevice, usb_interface_t *interface, uint8_t *interfaceConfBuffer, uint8_t *interfaceConfBufferEnd);

#endif
