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

static bool usb_device_print_interface(usb_device_t *usbDevice, uint16_t langId, usb_descriptor_interface_t *interfaceDesc) {
    // Get string.
    char *strInterface;
    if (!usb_device_get_string(usbDevice, langId, interfaceDesc->InterfaceString, &strInterface)) {
        return false;
    }

    // Print info.
    kprintf("USB: Interface %u:\n", interfaceDesc->IntefaceNumber);
    kprintf("USB:     Alternate setting: %u\n", interfaceDesc->AlternateSetting);
    kprintf("USB:     Endpoints: %u\n", interfaceDesc->NumEndpoints);
    kprintf("USB:     Class: 0x%X | Subclass: 0x%X | Protocol: 0x%X\n", interfaceDesc->InterfaceClass,
        interfaceDesc->InterfaceSubclass, interfaceDesc->InterfaceProtocol);
    kprintf("USB:     Interface string: %s\n", strInterface);

    // Free string.
    kheap_free(strInterface);
    return true;
}

static void usb_device_print_endpoint(usb_descriptor_endpoint_t *endpointDesc) {
    kprintf("USB: Endpoint %u:\n", endpointDesc->EndpointNumber);
    if (endpointDesc->TransferType != USB_ENDPOINT_TRANSFERTYPE_CONTROL)
        kprintf("USB:     Direction: %s\n", endpointDesc->Inbound ? "IN" : "OUT");

    // Print transfer type.
    char *transferType = "";
    switch (endpointDesc->TransferType) {
        case USB_ENDPOINT_TRANSFERTYPE_CONTROL:
            transferType = "control";
            break;
        
        case USB_ENDPOINT_TRANSFERTYPE_ISO:
            transferType = "isochronous";
            break;

        case USB_ENDPOINT_TRANSFERTYPE_BULK:
            transferType = "bulk";
            break;

        case USB_ENDPOINT_TRANSFERTYPE_INTERRUPT:
            transferType = "interrupt";
            break;
    }
    kprintf("USB:     Transfer type: %s\n", transferType);

    // Print sync type.
    char *syncType = "";
    switch (endpointDesc->SyncType) {
        case USB_ENDPOINT_SYNCTYPE_NONE:
            syncType = "none";
            break;

        case USB_ENDPOINT_SYNCTYPE_ASYNC:
            syncType = "asynchronous";
            break;

        case USB_ENDPOINT_SYNCTYPE_ADAPTIVE:
            syncType = "adaptive";
            break;

        case USB_ENDPOINT_SYNCTYPE_SYNC:
            syncType = "synchronous";
            break;
    }
    kprintf("USB:     Synchronization type: %s\n", syncType);

    // Print usage type.
    char *usageType = "";
    switch (endpointDesc->UsageType) {
        case USB_ENDPOINT_USAGETYPE_DATA:
            usageType = "data";
            break;

        case USB_ENDPOINT_USAGETYPE_FEEDBACK:
            usageType = "feedback";
            break;

        case USB_ENDPOINT_USAGETYPE_IMPL_FEEDBACK:
            usageType = "implicit feedback";
            break;
    }
    kprintf("USB:     Usage type: %s\n", usageType);

    // Print additional inof.
    kprintf("USB:     Max packet size: %u bytes\n", endpointDesc->MaxPacketSize);
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
    
    // Allocate address for device.
    if (!usbDevice->AllocAddress(usbDevice))
        return false;

    // Get full device descriptor.
    memset(&deviceDesc, 0, sizeof(usb_descriptor_device_t));
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_DEVICE, 0, &deviceDesc, sizeof(usb_descriptor_device_t)))
        return false;

    // Get strings.
    uint16_t langId = 0;
    if (!(usb_device_get_first_lang(usbDevice, &langId)))
        kprintf("USB: Strings are not supported by this device.\n");

    char *strVendor;
    char *strProduct;
    char *strSerial;
    if (!usb_device_get_string(usbDevice, langId, deviceDesc.VendorString, &strVendor))
        return false;
    if (!usb_device_get_string(usbDevice, langId, deviceDesc.ProductString, &strProduct)) {
        kheap_free(strVendor);
        return false;
    }
    if (!usb_device_get_string(usbDevice, langId, deviceDesc.SerialNumberString, &strSerial)) {
        kheap_free(strVendor);
        kheap_free(strProduct);
        return false;
    }

    // Print information.
    kprintf("USB: Initialized device (address %u)!\n", usbDevice->Address);
    kprintf("USB: Device data:\n");
    kprintf("USB:     Vendor: %s\n", strVendor);
    kprintf("USB:     Product: %s\n", strProduct);
    kprintf("USB:     Serial number: %s\n", strSerial);
    kprintf("USB:     Version: %u.%u | Max packet size: %u bytes\n", deviceDesc.BcdUsb >> 8, deviceDesc.BcdUsb & 0xFF, usbDevice->MaxPacketSize);
    kprintf("USB:     Vendor ID: 0x%X | Product ID: 0x%X | Configs: %u\n", deviceDesc.VendorId, deviceDesc.ProductId, deviceDesc.NumConfigurations);

    // Clean up strings.
    kheap_free(strVendor);
    kheap_free(strProduct);
    kheap_free(strSerial);

    // Get the first configuration.
    usb_descriptor_configuration_t confDesc = { };
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, &confDesc, sizeof(usb_descriptor_configuration_t)))
        return false;

    // Get full configuration data (configuration descriptor + interface/endpoint descriptors).
    uint8_t *confBuffer = kheap_alloc(confDesc.TotalLength);
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_CONFIGURATION, 0, confBuffer, confDesc.TotalLength)) {
        kheap_free(confBuffer);
        return false;
    }

    char *strConfiguration;
    if (!usb_device_get_string(usbDevice, langId, confDesc.ConfigurationString, &strConfiguration))
        return false;

    kprintf("USB: Configuration 0 data:\n");
    kprintf("USB:     Interfaces: %u\n", confDesc.NumInterfaces);
    kprintf("USB:     Configuration value: 0x%X\n", confDesc.ConfigurationValue);
    kprintf("USB:     Configuration string: %s\n", strConfiguration);
    kprintf("USB:     Self-powered: %s | Remote wakeup: %s\n", confDesc.SelfPowered ? "yes" : "no", confDesc.RemoteWakeup ? "yes" : "no");
    kprintf("USB:     Max bus power used: %u mA\n", confDesc.MaxPower * USB_MAX_POWER_UNITS);
    kheap_free(strConfiguration);

    // Search configuration data for interfaces and endpoints.
    uint8_t *currentConfBuffer = confBuffer + sizeof(usb_descriptor_configuration_t);
    uint8_t *endConfBuffer = confBuffer + confDesc.TotalLength;
    while (currentConfBuffer < endConfBuffer) {
        // Get length and type.
        uint8_t length = currentConfBuffer[0];
        uint8_t type = currentConfBuffer[1];

        // Check type.
        kprintf("USB: Got descriptor 0x%X, %u bytes in length\n", type, length);
        if (type == USB_DESCRIPTOR_TYPE_INTERFACE) {
            // Get pointer to interface descriptor and print info.
            usb_descriptor_interface_t *interfaceDesc = (usb_descriptor_interface_t*)currentConfBuffer;
            if (!usb_device_print_interface(usbDevice, langId, interfaceDesc)) {
                kheap_free(confBuffer);
                return false;
            }
        }
        else if (type == USB_DESCRIPTOR_TYPE_ENDPOINT) {
            // Get pointer to endpoint descriptor and print info.
            usb_descriptor_endpoint_t *endpointDesc = (usb_descriptor_endpoint_t*)currentConfBuffer;
            usb_device_print_endpoint(endpointDesc);
        }
        else {
            kprintf("USB: Ignoring other descriptor 0x%X\n", type);
        }

        // Move to next descriptor.
        currentConfBuffer += length;
    }

    // Save configuration value for later use.
    usbDevice->ConfigurationValue = confDesc.ConfigurationValue;
    usbDevice->Configured = false;

    // Search configuration data for interfaces, and load drivers.
    currentConfBuffer = confBuffer + sizeof(usb_descriptor_configuration_t);
    endConfBuffer = confBuffer + confDesc.TotalLength;
    while (currentConfBuffer < endConfBuffer) {
        // Get length and type.
        uint8_t length = currentConfBuffer[0];
        uint8_t type = currentConfBuffer[1];

        // Make sure we have an interface descriptor.
        if (type == USB_DESCRIPTOR_TYPE_INTERFACE) {
            // Get pointer to interface descriptor.
            usb_descriptor_interface_t *interfaceDesc = (usb_descriptor_interface_t*)currentConfBuffer;
            
            if (interfaceDesc->InterfaceClass == USB_CLASS_HUB && interfaceDesc->InterfaceSubclass == 0x00) {
                usb_hub_init(usbDevice, interfaceDesc);
            }
        }

        // Move to next descriptor.
        currentConfBuffer += length;
    }

    return true;
}

bool usb_device_configure(usb_device_t *usbDevice) {
    // Set configuration.
    kprintf("USB: Setting configuration on device %u to %u...\n", usbDevice->Address, usbDevice->ConfigurationValue);
    if (!usbDevice->ControlTransfer(usbDevice, 0, false, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_SET_CONFIG, usbDevice->ConfigurationValue, 0, 0, NULL, 0))
        return false;
    
    // Get configuration.
    uint8_t newConfigurationValue = 0;
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_STANDARD,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_CONFIG, 0, 0, 0, &newConfigurationValue, sizeof(newConfigurationValue)))
        return false;

    // Ensure current config value matches desired value.
    bool result = usbDevice->ConfigurationValue == newConfigurationValue;
    if (result)
        kprintf("USB: Device %u configured!\n", usbDevice->Address);
    return result;
}
