#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

#include <kernel/memory/kheap.h>

usb_device_t *usb_device_create() {
    // Allocate space for a USB device and return it.
    usb_device_t *device = (usb_device_t*)kheap_alloc(sizeof(usb_device_t));
    memset(device, 0, sizeof(usb_device_t));
    return device;
}

bool usb_device_get_first_lang(usb_device_t *usbDevice, uint16_t *outLang) {
    // Create temporary string descriptor.
    usb_descriptor_string_t stringDesc = {};

    // Get first language code.
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_STRING, 0, &stringDesc, sizeof(usb_descriptor_string_t)))
        return false;
    *outLang = stringDesc.String[0];
    return true;
}

bool usb_device_get_string(usb_device_t *usbDevice, uint16_t langId, uint8_t index, char **outStr) {
    // If the index is zero, simply return a null string.
    if (!index) {
        *outStr = kheap_alloc(1);
        if (*outStr == NULL)
            return false;

        // Null string.
        char* str = *outStr;
        str[0] = '\0';
        return true;
    }

    // Get length of string descriptor using temporary one.
    usb_descriptor_string_t tempStringDesc = { };
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_TYPE_STRING, langId, &tempStringDesc, sizeof(usb_descriptor_string_t)))
        return false;
    
    // Allocate space for full descriptor and get it.
    usb_descriptor_string_t *stringDesc = (usb_descriptor_string_t*)kheap_alloc(tempStringDesc.Length);
    memset(stringDesc, 0, tempStringDesc.Length);
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_TYPE_STRING, langId, stringDesc, tempStringDesc.Length)) {
        kheap_free(stringDesc);
        return false;
    }

    // Get length of string (pull off first two bytes, and each unicode character = 2 bytes).
    uint16_t strLength = (stringDesc->Length - 2) / 2;
    *outStr = kheap_alloc(strLength + 1); // Length plus null terminator.
    if (*outStr == NULL)
        return false;

    // Convert unicode to char.
    char* str = *outStr;
    for (uint16_t i = 0; i < strLength; i++)
        str[i] = (char)stringDesc->String[i];
    str[strLength] = '\0';
    return true;
}

bool usb_device_init(usb_device_t *usbDevice) {
    // Create descriptor object.
    usb_descriptor_device_t deviceDesc = {};

    // Get first 8 bytes of device descriptor. This will tell us the max packet size.
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_DEVICE, 0, &deviceDesc, 8))
        return false;

    // Set max packet size.
    usbDevice->MaxPacketSize = deviceDesc.MaxPacketSize;

    // Get the next available address from the controller and set it.
    uint8_t address = 3;
    if (!usbDevice->ControlTransfer(usbDevice, 0, false, USB_REQUEST_TYPE_STANDARD, USB_REQUEST_REC_DEVICE, USB_REQUEST_SET_ADDRESS, address, 0, 0, NULL, 0))
        return false;
    usbDevice->Address = address;
    sleep(1);

    // Get full device descriptor.
    memset(&deviceDesc, 0, sizeof(usb_descriptor_device_t));
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_DEVICE, 0, &deviceDesc, sizeof(usb_descriptor_device_t)))
        return false;



    // Get strings.
    uint16_t langId = 0;
    if (!usb_device_get_first_lang(usbDevice, &langId))
        return false;

    char *strVendor;
    char *strProduct;
    char *strSerial;
    usb_device_get_string(usbDevice, langId, deviceDesc.VendorString, &strVendor);
    usb_device_get_string(usbDevice, langId, deviceDesc.ProductString, &strProduct);
    usb_device_get_string(usbDevice, langId, deviceDesc.SerialNumberString, &strSerial);

    // Print information.
    kprintf("USB: Initialized device (%u)!\n", usbDevice->Address);
    kprintf("USB: Vendor: %s\n", strVendor);
    kprintf("USB: Product: %s\n", strProduct);
    kprintf("USB: Serial number: %s\n", strSerial);
    kprintf("USB: Version: %u.%u    Max packet size: %u bytes\n", deviceDesc.BcdUsb >> 8, deviceDesc.BcdUsb & 0xFF, usbDevice->MaxPacketSize);
    kprintf("USB: Vendor ID: 0x%X    Product ID: 0x%X     Configs: %u\n", deviceDesc.VendorId, deviceDesc.ProductId, deviceDesc.NumConfigurations);
}