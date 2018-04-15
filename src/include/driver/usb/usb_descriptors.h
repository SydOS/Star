#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <main.h>

#define USB_DESCRIPTOR_TYPE_DEVICE              1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION       2
#define USB_DESCRIPTOR_TYPE_STRING              3
#define USB_DESCRIPTOR_TYPE_INTERFACE           4
#define USB_DESCRIPTOR_TYPE_ENDPOINT            5
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER    6
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONF    7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER     8

// USB device descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H).
    // This field identifies the release of the USB Specification with which the device
    // and its descriptors are compliant.
    uint16_t BcdUsb;

    // Class code (assigned by the USB-IF).
    uint8_t Class;

    // Subclass code (assigned by the USB-IF).
    uint8_t Subclass;

    // Protocol code (assigned by the USB-IF).
    uint8_t Protocol;

    // Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid).
    uint8_t MaxPacketSize;

    // Vendor ID (assigned by the USB-IF).
    uint16_t VendorId;

    // Product ID (assigned by the manufacturer).
    uint16_t ProductId;

    // Device release number in binary-coded decimal.
    uint16_t BcdDevice;

    // String descriptor indexes.
    uint8_t VendorString;
    uint8_t ProductString;
    uint8_t SerialNumberString;

    // Number of possible configurations.
    uint8_t NumConfigurations;
} __attribute__((packed)) usb_descriptor_device_t;

// USB device qualifier descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H).
    // This field identifies the release of the USB Specification with which the device
    // and its descriptors are compliant.
    uint16_t BcdUsb;

    // Class code (assigned by the USB-IF).
    uint8_t Class;

    // Subclass code (assigned by the USB-IF).
    uint8_t Subclass;

    // Protocol code (assigned by the USB-IF).
    uint8_t Protocol;

    // Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid).
    uint8_t MaxPacketSize;

    // Number of possible configurations.
    uint8_t NumConfigurations;

    // Reserved, must be zero.
    uint8_t Reserved;
} __attribute__((packed)) usb_descriptor_qualifier_t;

// USB configuration descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // Total length of data returned for this configuration.
    uint16_t TotalLength;

    // Number of possible configurations.
    uint8_t NumConfigurations;

    // Value to use as an argument to the SetConfiguration() request to select this configuration.
    uint8_t ConfigurationValue;

    // Index of string descriptor describing this configuration.
    uint8_t ConfigurationString;

    // Configuration characteristics.
    bool Reserved1 : 1; // Must be set to one.
    bool SelfPowered : 1;
    bool RemoteWakeup : 1;
    uint8_t Reserved2 : 5;

    // Maximum power consumption.
    uint8_t MaxPower;
} __attribute__((packed)) usb_descriptor_configuration_t;

// USB interface descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // Number of this interface.
    uint8_t IntefaceNumber;

    // Value used to select this alternate setting for the interface identified in the prior field.
    uint8_t AlternateSetting;

    // Number of endpoints used by this interface (excluding endpoint zero).
    uint8_t NumEndpoints;

    // Class code (assigned by the USB-IF).
    uint8_t InterfaceClass;

    // Subclass code (assigned by the USB-IF).
    uint8_t InterfaceSubclass;

    // Protocol code (assigned by the USB).
    uint8_t InterfaceProtocol;

    // Index of string descriptor describing this interface.
    uint8_t InterfaceString;
} __attribute__((packed)) usb_descriptor_interface_t;

// USB endpoint descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // The address of the endpoint on the USB device described by this descriptor.
    uint8_t EndpointAddress;

    uint8_t Attributes;

    // Maximum packet size this endpoint is capable of sending or receiving when
    // this configuration is selected.
    uint16_t MaxPacketSize;

    // Interval for polling endpoint for data transfers.
    uint8_t Interval;
} __attribute__((packed)) usb_descriptor_endpoint_t;

// USB string descriptor.
typedef struct {
    // Size of this descriptor in bytes.
    uint8_t Length;

    // Descriptor type.
    uint8_t Type;

    // String.
    uint16_t String[];
} __attribute__((packed)) usb_descriptor_string_t;

#endif
