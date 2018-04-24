#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/devices/usb_device.h>
#include <driver/usb/devices/usb_driver.h>
#include <driver/usb/devices/usb_requests.h>
#include <driver/usb/devices/usb_descriptors.h>

#include <kernel/memory/kheap.h>

usb_device_t *StartUsbDevice = NULL;

static char *usbClassDescriptions[255];
static bool usbClassDescriptionsFilled = false;

static void usb_fill_class_names(void) {
    usbClassDescriptions[0x00] = "Defined in interface";
    usbClassDescriptions[0x01] = "Audio device";
    usbClassDescriptions[0x02] = "Communications device";
    usbClassDescriptions[0x03] = "Human Interface Device";
    usbClassDescriptions[0x05] = "Physical device";
    usbClassDescriptions[0x06] = "Still imaging device";
    usbClassDescriptions[0x07] = "Printing device";
    usbClassDescriptions[0x08] = "Mass storage device";
    usbClassDescriptions[0x09] = "Hub";
    usbClassDescriptions[0x0A] = "Communications data device";
    usbClassDescriptions[0x0B] = "Smart card controller";
    usbClassDescriptions[0x0D] = "Content security device";
    usbClassDescriptions[0x0E] = "Video device";
    usbClassDescriptions[0x0F] = "Personal healthcare device";
    usbClassDescriptions[0x10] = "Audio/video device";
    usbClassDescriptions[0x11] = "Billboard device";
    usbClassDescriptions[0x12] = "USB Type-C bridge device";
    usbClassDescriptions[0xDC] = "Diagnostic device";
    usbClassDescriptions[0xE0] = "Wireless controller";
    usbClassDescriptions[0xEF] = "Miscellaneous device";
    usbClassDescriptions[0xFE] = "Application-specific device";
    usbClassDescriptions[0xFF] = "Vendor-specific device";
    usbClassDescriptionsFilled = true;
}

bool usb_device_get_first_lang(usb_device_t *usbDevice, uint16_t *outLang) {
    // Create temporary string descriptor.
    usb_descriptor_string_t stringDesc = {};

    // Build transfer to get language codes.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_DESCRIPTOR;
    transfer.ValueHigh = USB_DESCRIPTOR_TYPE_STRING;

    // Get first language code.
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &stringDesc, sizeof(usb_descriptor_string_t)))
        return false;
    *outLang = stringDesc.String[0];
    return true;
}

bool usb_device_get_string(usb_device_t *usbDevice, uint16_t langId, uint8_t index, char *defaultStr, char **outStr) {
    // If the index is zero, simply return a null string.
    if (!index) {
        *outStr = kheap_alloc(strlen(defaultStr));
        if (*outStr == NULL)
            return false;

        // Use default string.
        char* str = *outStr;
        strcpy(str, defaultStr);
        return true;
    }

    // Build transfer to get string descriptor.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_DESCRIPTOR;
    transfer.ValueLow = index;
    transfer.ValueHigh = USB_DESCRIPTOR_TYPE_STRING;
    transfer.Index = langId;

    // Get length of string descriptor using temporary one.
    usb_descriptor_string_t tempStringDesc = { };
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &tempStringDesc, sizeof(usb_descriptor_string_t)))
        return false;

    // Allocate space for full descriptor and get it.
    usb_descriptor_string_t *stringDesc = (usb_descriptor_string_t*)kheap_alloc(tempStringDesc.Length);
    memset(stringDesc, 0, tempStringDesc.Length);
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, stringDesc, tempStringDesc.Length)) {
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
    if (!usb_device_get_string(usbDevice, langId, interfaceDesc->InterfaceString, "", &strInterface)) {
        return false;
    }

    // Print info.
    kprintf("USB: Interface %u:\n", interfaceDesc->IntefaceNumber);
    kprintf("USB:     Type: %s\n", usbClassDescriptions[interfaceDesc->InterfaceClass]);
    kprintf("USB:     Class: 0x%X | Subclass: 0x%X | Protocol: 0x%X\n", interfaceDesc->InterfaceClass,
        interfaceDesc->InterfaceSubclass, interfaceDesc->InterfaceProtocol);
    kprintf("USB:     Alternate setting: %u\n", interfaceDesc->AlternateSetting);
    kprintf("USB:     Endpoints: %u\n", interfaceDesc->NumEndpoints);
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

usb_device_t *usb_device_create(usb_device_t *parentDevice, uint8_t port, uint8_t speed) {
    // Allocate space for a USB device.
    usb_device_t *usbDevice = (usb_device_t*)kheap_alloc(sizeof(usb_device_t));
    memset(usbDevice, 0, sizeof(usb_device_t));

    // Inherit controller info from parent.
    usbDevice->Parent = parentDevice;
    usbDevice->Controller = parentDevice->Controller;
    usbDevice->AllocAddress = parentDevice->AllocAddress;
    usbDevice->FreeAddress = parentDevice->FreeAddress;
    usbDevice->ControlTransfer = parentDevice->ControlTransfer;
    usbDevice->InterruptTransferStart = parentDevice->InterruptTransferStart;
    usbDevice->InterruptTransferPoll = parentDevice->InterruptTransferPoll;
    usbDevice->InterruptTransferStop = parentDevice->InterruptTransferStop;

    // Set port and speed. Address is 0 as the device is in setup mode.
    usbDevice->Address = 0;
    usbDevice->Port = port;
    usbDevice->Speed = speed;

    // Create endpoint zero.
    usbDevice->EndpointZero = (usb_endpoint_t*)kheap_alloc(sizeof(usb_endpoint_t));
    memset(usbDevice->EndpointZero, 0, sizeof(usb_endpoint_t));
    usbDevice->EndpointZero->Number = 0;
    usbDevice->EndpointZero->Type = USB_ENDPOINT_TRANSFERTYPE_CONTROL;
    usbDevice->EndpointZero->MaxPacketSize = 8;

    // Build transfer to get device descriptor.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_DESCRIPTOR;
    transfer.ValueLow = 0;
    transfer.ValueHigh = USB_DESCRIPTOR_TYPE_DEVICE;
    transfer.Index = 0;

    // Create descriptor object.
    usb_descriptor_device_t deviceDesc = {};

    // Get first 8 bytes of device descriptor. This will tell us the max packet size.
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &deviceDesc, usbDevice->EndpointZero->MaxPacketSize)) {
        // The command failed, so destroy device and return nothing.
        usb_device_destroy(usbDevice);
        return NULL;
    }

    // Set max packet size of endpoint zero.
    usbDevice->EndpointZero->MaxPacketSize = deviceDesc.MaxPacketSize;

    // Get full device descriptor.
    deviceDesc = (usb_descriptor_device_t) { };
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &deviceDesc, sizeof(usb_descriptor_device_t))) {
        // The command failed, so destroy device and return nothing.
        usb_device_destroy(usbDevice);
        return NULL;
    }

    // Save info.
    usbDevice->VendorId = deviceDesc.VendorId;
    usbDevice->ProductId = deviceDesc.ProductId;
    usbDevice->Class = deviceDesc.Class;
    usbDevice->Subclass = deviceDesc.Subclass;
    usbDevice->Protocol = deviceDesc.Protocol;
    usbDevice->NumConfigurations = deviceDesc.NumConfigurations;

    // Get default language ID.
    uint16_t langId = 0;
    if (!(usb_device_get_first_lang(usbDevice, &langId)))
        kprintf("USB: Strings are not supported by this device.\n");

    // Fill class names if needed.
    if (!usbClassDescriptionsFilled)
        usb_fill_class_names();

    // Get strings.
    if (!usb_device_get_string(usbDevice, langId, deviceDesc.VendorString, USB_VENDOR_GENERIC, &usbDevice->VendorString)
        || !usb_device_get_string(usbDevice, langId, deviceDesc.ProductString, usbDevice->Class == USB_CLASS_HUB ? USB_HUB_GENERIC : USB_PRODUCT_GENERIC, &usbDevice->ProductString)
        || !usb_device_get_string(usbDevice, langId, deviceDesc.SerialNumberString, USB_SERIAL_GENERIC, &usbDevice->SerialNumber)) {
        // The command failed, so destroy device and return nothing.
        usb_device_destroy(usbDevice);
        return NULL;
    }

    // Allocate address for device.
    kprintf("USB: Allocating address for new device...\n");
    if (!usbDevice->AllocAddress(usbDevice)) {
        // The command failed, so destroy device and return nothing.
        usb_device_destroy(usbDevice);
        return NULL;
    }

    // Build transfer to get configuration descriptor.
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_DESCRIPTOR;
    transfer.ValueLow = 0;
    transfer.ValueHigh = USB_DESCRIPTOR_TYPE_CONFIGURATION;
    transfer.Index = 0;

    // Get the first configuration.
    usb_descriptor_configuration_t confDesc = { };
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &confDesc, sizeof(usb_descriptor_configuration_t)))
        return false;

    // Get full configuration data (configuration descriptor + interface/endpoint descriptors).
    kprintf("USB: Getting configuration data for new device...\n");
    uint8_t *confBuffer = kheap_alloc(confDesc.TotalLength);
    memset(confBuffer, 0, confDesc.TotalLength);
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, confBuffer, confDesc.TotalLength)) {
        kheap_free(confBuffer);
        return false;
    }

    // Save the config value for later use.
    usbDevice->CurrentConfigurationValue = confDesc.ConfigurationValue;
    usbDevice->Configured = false;

    // Search configuration data for interfaces, and load drivers.
    uint8_t *currentConfBuffer = confBuffer + sizeof(usb_descriptor_configuration_t);
    uint8_t *endConfBuffer = confBuffer + confDesc.TotalLength;
    while (currentConfBuffer < endConfBuffer) {
        // Get length and type.
        uint8_t length = currentConfBuffer[0];
        uint8_t type = currentConfBuffer[1];

        // Make sure we have an interface descriptor.
        if (type == USB_DESCRIPTOR_TYPE_INTERFACE) {
            // Get pointer to interface descriptor.
            usb_descriptor_interface_t *interfaceDesc = (usb_descriptor_interface_t*)currentConfBuffer;

            // Create USB interface object.
            usb_interface_t *interface = (usb_interface_t*)kheap_alloc(sizeof(usb_interface_t));
            memset(interface, 0, sizeof(usb_interface_t));
            interface->Number = interfaceDesc->IntefaceNumber;
            interface->Class = interfaceDesc->InterfaceClass;
            interface->Subclass = interfaceDesc->InterfaceSubclass;
            interface->Protocol = interfaceDesc->InterfaceProtocol;

            // Get length of current interface (search until we find the next interface, or the end).
            uint8_t *endInterfaceBuffer = currentConfBuffer + sizeof(usb_descriptor_interface_t);
            while (endInterfaceBuffer < endConfBuffer) {
                // Get length and type.
                uint8_t iLength = endInterfaceBuffer[0];
                uint8_t iType = endInterfaceBuffer[1];

                // If the descriptor is an interface, we are done.
                if (iType == USB_DESCRIPTOR_TYPE_INTERFACE)
                    break;
                
                // Move to next descriptor.
                endInterfaceBuffer += iLength;
            }

            // Search for endpoints.
            interface->NumEndpoints = 0;
            uint8_t *currentInterfaceBuffer = currentConfBuffer + sizeof(usb_descriptor_interface_t);
            while (currentInterfaceBuffer < endInterfaceBuffer) {
                // Get length and type.
                uint8_t iLength = currentInterfaceBuffer[0];
                uint8_t iType = currentInterfaceBuffer[1];

                // If the descriptor is an interface, we are done.
                if (iType == USB_DESCRIPTOR_TYPE_INTERFACE)
                    break;
                // Do we have an endpoint?
                else if (iType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
                    // Get endpoint descriptor.
                    usb_descriptor_endpoint_t *endpointDesc = (usb_descriptor_endpoint_t*)currentInterfaceBuffer;

                    // Create endpoint object.
                    usb_endpoint_t *endpoint = (usb_endpoint_t*)kheap_alloc(sizeof(usb_endpoint_t));
                    memset(endpoint, 0, sizeof(usb_endpoint_t));
                    endpoint->Number = endpointDesc->EndpointNumber;
                    endpoint->Inbound = endpointDesc->Inbound;
                    endpoint->Type = endpointDesc->TransferType;
                    endpoint->Interval = endpointDesc->Interval;
                    endpoint->MaxPacketSize = endpointDesc->MaxPacketSize; // TODO fix

                    // Add endpoint to interface.
                    interface->NumEndpoints++;
                    interface->Endpoints = (usb_endpoint_t**)kheap_realloc(interface->Endpoints, interface->NumEndpoints * sizeof(usb_endpoint_t));
                    interface->Endpoints[interface->NumEndpoints - 1] = endpoint;
                }

                // Move to next descriptor.
                currentInterfaceBuffer += iLength;
            }

            // Attempt to install driver for interface.
            uint16_t driverIndex = 0;
            while (UsbDrivers[driverIndex].Initialize != NULL) {
                if (UsbDrivers[driverIndex].Initialize(usbDevice, interface, currentConfBuffer, endInterfaceBuffer))
                    break;
                driverIndex++;
            }
        }

        // Move to next descriptor.
        currentConfBuffer += length;
    }

    return usbDevice;
}

void usb_device_destroy(usb_device_t *usbDevice) {
    // Free strings.
    kprintf("USB: Freeing strings...\n");
    if (usbDevice->VendorString != NULL)
        kheap_free(usbDevice->VendorString);
    if (usbDevice->ProductString != NULL)
        kheap_free(usbDevice->ProductString);
    if (usbDevice->SerialNumber != NULL)
        kheap_free(usbDevice->SerialNumber);

    // Free each interface.
    if (usbDevice->Interfaces != NULL) {
        for (uint8_t interfaceIndex = 0; interfaceIndex < usbDevice->NumInterfaces; interfaceIndex++) {
            // Get interface.
            usb_interface_t *interface = usbDevice->Interfaces[interfaceIndex];

            // Free each endpoint.
            if (interface->Endpoints != NULL) {
                for (uint8_t endpointIndex = 0; endpointIndex < interface->NumEndpoints; endpointIndex++)
                    kheap_free(interface->Endpoints[endpointIndex]);
                kheap_free(interface->Endpoints);
            }

            // Free driver, if any.
            if (interface->Driver != NULL)
                kheap_free(interface->Driver);

            // Free interface.
            kheap_free(interface);
        }
        kheap_free(usbDevice->Interfaces);
    }

    // Destroy endpoint zero.
    if (usbDevice->EndpointZero != NULL)
        kheap_free(usbDevice->EndpointZero);

    // Free device.
    kheap_free(usbDevice);
}

bool usb_device_configure(usb_device_t *usbDevice) {
    // Build transfer to set device configuration.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = false;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_SET_CONFIG;
    transfer.ValueLow = usbDevice->CurrentConfigurationValue;
    transfer.ValueHigh = 0;
    transfer.Index = 0;

    // Set configuration.
    kprintf("USB: Setting configuration on device %u to config %u...\n", usbDevice->Address, usbDevice->CurrentConfigurationValue);
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, NULL, 0))
        return false;

    // Build transfer to get device configuration.
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_STANDARD;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_CONFIG;
    transfer.ValueLow = 0;
    transfer.ValueHigh = 0;
    transfer.Index = 0;

    // Get configuration.
    uint8_t newConfigurationValue = 0;
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &newConfigurationValue, sizeof(newConfigurationValue)))
        return false;

    // Ensure current config value matches desired value.
    bool result = usbDevice->CurrentConfigurationValue == newConfigurationValue;
    if (result)
        kprintf("USB: Device %u configured!\n", usbDevice->Address);
    return result;
}
