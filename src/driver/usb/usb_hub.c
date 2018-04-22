#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_hub.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

#include <kernel/memory/kheap.h>

void usb_hub_print_desc(usb_descriptor_hub_t *hubDesc) {
    kprintf("USB HUB:     Ports: %u\n", hubDesc->NumPorts);
    kprintf("USB HUB:     Port indicators: %s\n", hubDesc->PortIndicatorsSupported ? "yes" : "no");
    kprintf("USB HUB:     Part of compound device: %s\n", hubDesc->CompoundDevice ? "yes" : "no");

    // Get power switching mode.
    char *powerMode = "none";
    switch (hubDesc->PowerSwitchingMode) {
        case USB_HUB_POWERSW_GANGED:
            powerMode = "ganged";
            break;

        case USB_HUB_POWERSW_INDIV:
            powerMode = "individual";
            break;
    }
    kprintf("USB HUB:     Power switching mode: %s\n", powerMode);

    // Get over-current protection mode.
    char *overCurrentMode = "none";
    switch (hubDesc->OvercurrentMode) {
        case USB_HUB_OVERCURRENT_GLOBAL:
            overCurrentMode = "global";
            break;

        case USB_HUB_OVERCURRENT_INDIV:
            overCurrentMode = "individual";
            break;
    }
    kprintf("USB HUB:     Over-current protection mode: %s\n", overCurrentMode);

    // Print rest of info.
    kprintf("USB HUB:     Time until port power is stable: %u ms\n", hubDesc->PowerOnToPowerGood * USB_HUB_POWERTOPORT_UNITS);
    kprintf("USB HUB:     Max power required by controller: %u mA\n", hubDesc->HubControllerCurrent);
}

static usb_hub_port_status_t usb_hub_reset_port(usb_hub_t *usbHub, uint8_t port) {
    // Build transfer to reset port.
    usb_control_transfer_t transferReset = { };
    transferReset.Inbound = false;
    transferReset.Type = USB_REQUEST_TYPE_CLASS;
    transferReset.Recipient = USB_REQUEST_REC_OTHER;
    transferReset.Request = USB_REQUEST_SET_FEATURE;
    transferReset.ValueLow = USB_HUB_FEAT_PORT_RESET;
    transferReset.ValueHigh = 0;
    transferReset.Index = port;

    // Build transfer to get port status.
    usb_control_transfer_t transferStatus = { };
    transferStatus.Inbound = true;
    transferStatus.Type = USB_REQUEST_TYPE_CLASS;
    transferStatus.Recipient = USB_REQUEST_REC_OTHER;
    transferStatus.Request = USB_REQUEST_GET_STATUS;
    transferStatus.ValueLow = 0;
    transferStatus.ValueHigh = 0;
    transferStatus.Index = port;

    // Send reset command to hub.
    kprintf("USB HUB: Resetting port %u on hub %u...\n", port, usbHub->Device->Address);
    if (!usbHub->Device->ControlTransfer(usbHub->Device, usbHub->Device->EndpointZero, transferReset, NULL, 0))
        return (const usb_hub_port_status_t) { };

    // Wait for port to enable.
    kprintf("USB HUB: Port %u reset! Waiting for port...\n", port);
    usb_hub_port_status_t status = { };
    for (uint8_t i = 0; i < 10; i++) {
        // Wait 10ms.
        sleep(10);

        // Get current status of port.
        if (!usbHub->Device->ControlTransfer(usbHub->Device, usbHub->Device->EndpointZero, transferStatus, &status, sizeof(usb_hub_port_status_t)))
            return (const usb_hub_port_status_t) { };

        // Check if the port is disconnected, or enabled. If so, we are done.
        if (!status.Connected || status.Enabled)
            break;
    }

    // Return the status.
    return status;
}

static void usb_hub_probe(usb_hub_t *usbHub) {
    // Get port count.
    uint8_t portCount = usbHub->Descriptor->NumPorts;

    // Enable power on ports if each port is individually managed.
    if (usbHub->Descriptor->PowerSwitchingMode == USB_HUB_POWERSW_INDIV) {
        // Build transfer to power up port.
        usb_control_transfer_t transferPower = { };
        transferPower.Inbound = false;
        transferPower.Type = USB_REQUEST_TYPE_CLASS;
        transferPower.Recipient = USB_REQUEST_REC_OTHER;
        transferPower.Request = USB_REQUEST_SET_FEATURE;
        transferPower.ValueLow = USB_HUB_FEAT_PORT_POWER;
        transferPower.ValueHigh = 0;

        // Power up ports.
        for (uint8_t port = 1; port <= portCount; port++) {
            // Enable power for port. If it results in failure, abort probe.
            kprintf("USB HUB: Enabling power for port %u...\n", port);
            transferPower.Index = port;
            if (!usbHub->Device->ControlTransfer(usbHub->Device, usbHub->Device->EndpointZero, transferPower, NULL, 0))
                return;
            kprintf("USB HUB: Power enabled for port %u!\n", port);
        }

        // Wait for ports to power up.
        sleep(usbHub->Descriptor->PowerOnToPowerGood * USB_HUB_POWERTOPORT_UNITS);
    }

    // Reset ports and enumerate devices on those ports.
    for (uint8_t port = 1; port <= portCount; port++) {
        for (uint8_t i = 0; i < 5; i++) {
            // Reset port.
            usb_hub_port_status_t status = usb_hub_reset_port(usbHub, port);
            sleep(20);

            // If the port is enabled, intialize device on port.
            if (status.Enabled) {
                // Determine speed.lsus
                uint8_t speed = USB_SPEED_FULL;
                if (status.LowSpeed)
                    speed = USB_SPEED_LOW;
                else if (status.HighSpeed)
                    speed = USB_SPEED_HIGH;

                // Initialize device.
                kprintf("USB HUB: Initializing new device on hub %u...\n", usbHub->Device->Address);
                usb_device_t *usbDevice = usb_device_create(usbHub->Device, port, speed);

                // If device is null, reset port and try again.
                if (usbDevice == NULL) {
                    kprintf("USB HUB: Failed! Retrying...\n");
                    sleep(500);
                    continue;
                }

                // Add as our child.
                if (usbHub->Device->Children != NULL) {
                    usb_device_t *lastDevice = usbHub->Device->Children;
                    while (lastDevice->Next != NULL)
                        lastDevice = lastDevice->Next;
                    lastDevice->Next = usbDevice;
                }
                else {
                    usbHub->Device->Children = usbDevice;
                }
            }

            // We are done, move to next port.
            break;
        }
    }
}

bool usb_hub_init(usb_device_t *usbDevice, usb_interface_t *interface, uint8_t *interfaceConfBuffer, uint8_t *interfaceConfBufferEnd) {
    // Ensure class code matches.
    if (interface->Class != USB_CLASS_HUB || interface->Subclass != 0)
        return false;

    // Search for status IN endpoint.
    usb_endpoint_t *endpointStatus = NULL;
    for (uint8_t e = 0; e < interface->NumEndpoints; e++) {
        usb_endpoint_t *endpoint = interface->Endpoints[e];
        if (endpoint->Inbound && endpoint->Type == USB_ENDPOINT_TRANSFERTYPE_INTERRUPT) {
            endpointStatus = endpoint;
            break;
        }
    }
    if (endpointStatus == NULL)
        return false;

    // Configure device if needed.
    kprintf("USB HUB: Initializing device %u...\n", usbDevice->Address);
    if (!usbDevice->Configured) {
        if (!usb_device_configure(usbDevice))
            return false;
        usbDevice->Configured = true;
    }

    // Build transfer to get hub descriptor.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = true;
    transfer.Type = USB_REQUEST_TYPE_CLASS;
    transfer.Recipient = USB_REQUEST_REC_DEVICE;
    transfer.Request = USB_REQUEST_GET_DESCRIPTOR;
    transfer.ValueLow = 0;
    transfer.ValueHigh = USB_DESCRIPTOR_TYPE_HUB;
    transfer.Index = 0;

    // Get hub descriptor length.
    uint8_t hubDescLength = 0;
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, &hubDescLength, sizeof(hubDescLength)))
        return false;

    // Get the hub descriptor.
    usb_descriptor_hub_t *hubDesc = (usb_descriptor_hub_t*)kheap_alloc(hubDescLength);
    memset(hubDesc, 0, hubDescLength);
    if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transfer, hubDesc, hubDescLength))
        return false;

    // Create USB hub object.
    usb_hub_t *usbHub = (usb_hub_t*)kheap_alloc(sizeof(usb_hub_t));
    memset(usbHub, 0, sizeof(usb_hub_t));
    usbHub->Device = usbDevice;
    usbHub->Descriptor = hubDesc;
    usbHub->StatusEndpoint = endpointStatus;

    // Print info.
    usb_hub_print_desc(usbHub->Descriptor);
    usb_hub_probe(usbHub);
    return true;
}