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
    // Send reset command to hub.
    kprintf("USB HUB: Resetting port %u on hub %u...\n", port, usbHub->Device->Address);
    if (!usbHub->Device->ControlTransfer(usbHub->Device, 0, false, USB_REQUEST_TYPE_CLASS,
        USB_REQUEST_REC_OTHER, USB_REQUEST_SET_FEATURE, USB_HUB_FEAT_PORT_RESET, 0, port, NULL, 0))
        return (const usb_hub_port_status_t) { };

    // Wait for port to enable.
    kprintf("USB HUB: Port %u reset! Waiting for port...\n", port);
    usb_hub_port_status_t status = { };
    for (uint8_t i = 0; i < 10; i++) {
        // Wait 10ms.
        sleep(10);

        // Get current status of port.
        if (!usbHub->Device->ControlTransfer(usbHub->Device, 0, true, USB_REQUEST_TYPE_CLASS,
            USB_REQUEST_REC_OTHER, USB_REQUEST_GET_STATUS, 0, 0, port, &status, sizeof(usb_hub_port_status_t)))
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
        for (uint8_t port = 1; port <= portCount; port++) {
            // If request fails, abort probe.
            kprintf("USB HUB: Enabling power for port %u...\n", port);
            if (!usbHub->Device->ControlTransfer(usbHub->Device, 0, false, USB_REQUEST_TYPE_CLASS,
                USB_REQUEST_REC_OTHER, USB_REQUEST_SET_FEATURE, USB_HUB_FEAT_PORT_POWER, 0, port, NULL, 0))
                return;
            kprintf("USB HUB: Power enabled for port %u!\n", port);
        }

        // Wait for ports to power up.
        sleep(usbHub->Descriptor->PowerOnToPowerGood * USB_HUB_POWERTOPORT_UNITS);
    }

    // Reset ports and enumerate devices on those ports.
    for (uint8_t port = 1; port <= portCount; port++) {
        // Reset port.
        usb_hub_port_status_t status = usb_hub_reset_port(usbHub, port);

        // If the port is enabled, intialize device on port.
        if (status.Enabled) {
            // Determine speed.
            uint8_t speed = USB_SPEED_FULL;
            if (status.LowSpeed)
                speed = USB_SPEED_LOW;
            else if (status.HighSpeed)
                speed = USB_SPEED_HIGH;

            // Create and intialize device.
            usb_device_t *usbDevice = usb_device_create();
            if (usbDevice != NULL) {
                 // This device is the parent.
                usbDevice->Parent = usbHub->Device;
                usbDevice->Controller = usbHub->Device->Controller;
                usbDevice->AllocAddress = usbHub->Device->AllocAddress;
                usbDevice->FreeAddress = usbHub->Device->FreeAddress;
                usbDevice->ControlTransfer = usbHub->Device->ControlTransfer;

                // Set port and speed.
                usbDevice->Port = port;
                usbDevice->Speed = speed;
                usbDevice->MaxPacketSize = 8;
                usbDevice->Address = 0;

                // Initialize device.
                kprintf("USB HUB: Initializing new device on hub %u...\n", usbHub->Device->Address);
                usb_device_init(usbDevice);
            }
        }
    }
}

bool usb_hub_init(usb_device_t *usbDevice, usb_descriptor_interface_t *interfaceDesc) {
    // Ensure class code matches.
    if (interfaceDesc->InterfaceClass != USB_CLASS_HUB)
        return false;

    // Get status endpoint.
    usb_descriptor_endpoint_t *statusEndpoint = (usb_descriptor_endpoint_t*)((uint8_t*)interfaceDesc + interfaceDesc->Length);
    if (statusEndpoint->Type != USB_DESCRIPTOR_TYPE_ENDPOINT || !statusEndpoint->Inbound || statusEndpoint->TransferType != USB_ENDPOINT_TRANSFERTYPE_INTERRUPT)
        return false;

    // Configure device if needed.
    kprintf("USB HUB: Initializing device %u...\n", usbDevice->Address);
    if (!usbDevice->Configured) {
        if (!usb_device_configure(usbDevice))
            return false;
        usbDevice->Configured = true;
    }

    // Get hub descriptor length.
    uint8_t hubDescLength = 0;
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_CLASS,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_HUB, 0, &hubDescLength, sizeof(hubDescLength)))
        return false;

    // Get the hub descriptor.
    usb_descriptor_hub_t *hubDesc = (usb_descriptor_hub_t*)kheap_alloc(hubDescLength);
    memset(hubDesc, 0, hubDescLength);
    if (!usbDevice->ControlTransfer(usbDevice, 0, true, USB_REQUEST_TYPE_CLASS,
        USB_REQUEST_REC_DEVICE, USB_REQUEST_GET_DESCRIPTOR, 0, USB_DESCRIPTOR_TYPE_HUB, 0, hubDesc, hubDescLength))
        return false;

    // Create USB hub object.
    usb_hub_t *usbHub = (usb_hub_t*)kheap_alloc(sizeof(usb_hub_t));
    memset(usbHub, 0, sizeof(usb_hub_t));
    usbHub->Device = usbDevice;
    usbHub->Descriptor = hubDesc;
    usbHub->StatusEndpointAddress = statusEndpoint->EndpointNumber;
    usbHub->StatusEndpointMaxPacketSize = statusEndpoint->MaxPacketSize;

    // Print info.
    usb_hub_print_desc(usbHub->Descriptor);
    usb_hub_probe(usbHub);
    return true;
}