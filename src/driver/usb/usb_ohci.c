#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <math.h>
#include <driver/usb/usb_ohci.h>

#include <driver/usb/devices/usb_descriptors.h>
#include <driver/usb/devices/usb_requests.h>
#include <driver/usb/devices/usb_device.h>

#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

static void *usb_ohci_alloc(usb_ohci_controller_t *controller, uint16_t size) {
    // Get total required blocks.
    uint16_t requiredBlocks = DIVIDE_ROUND_UP(size, USB_OHCI_MEM_BLOCK_SIZE);
    uint16_t currentBlocks = 0;
    uint16_t startIndex = 0;

    // Search for free block range.
    for (uint16_t i = 0; i < USB_OHCI_MEM_BLOCK_COUNT; i++) {
        // Have we found enough contigous blocks?
        if (currentBlocks >= requiredBlocks)
            break;

        // Is the block in-use?
        if (controller->MemMap[i]) {
            // Move to next one.
            startIndex = i + 1;
            currentBlocks = 0;
            continue;
        }

        // Add block.
        currentBlocks++;
    }

    if (currentBlocks < requiredBlocks)
        panic("OHCI: No more blocks!\n");

    // Mark blocks as used.
    for (uint16_t i = startIndex; i < startIndex + requiredBlocks; i++)
        controller->MemMap[i] = true;

    // Return pointer to blocks.
    return (void*)((uintptr_t)controller->HeapPool + (startIndex * 8));
}

static void usb_ohci_free(usb_ohci_controller_t *controller, void *pointer, uint16_t size) {
    // Ensure pointer is valid.
    if ((uintptr_t)pointer < (uintptr_t)controller->HeapPool || (uintptr_t)pointer > (uintptr_t)controller->HeapPool + USB_OHCI_MEM_POOL_SIZE || (uintptr_t)pointer & 0x7)
        panic("OHCI: Invalid pointer 0x%p free attempt!", pointer);

    // Get total required blocks.
    uint16_t blocks = DIVIDE_ROUND_UP(size, USB_OHCI_MEM_BLOCK_SIZE);
    uint16_t startIndex = ((uintptr_t)pointer - (uintptr_t)controller->HeapPool) / USB_OHCI_MEM_BLOCK_SIZE;

    // Mark blocks as free.
    for (uint16_t i = startIndex; i < startIndex + blocks; i++)
        controller->MemMap[i] = false;
}

static usb_ohci_transfer_desc_t *usb_ohci_endpoint_desc_alloc(usb_ohci_controller_t *controller, usb_ohci_endpoint_desc_t *previousDesc,
    usb_device_t *usbDevice, usb_endpoint_t *endpoint, usb_ohci_transfer_desc_t *headTransferDesc) {
    // Search for first free endpoint descriptor.
    for (usb_ohci_endpoint_desc_t *endpointDesc = controller->EndpointDescPool;
        (uintptr_t)endpointDesc + sizeof(usb_ohci_endpoint_desc_t) < (uintptr_t)controller->EndpointDescPool + USB_OHCI_ENDPOINT_POOL_SIZE; endpointDesc++) {
        if (!endpointDesc->InUse) {
            // Zero out descriptor and mark in-use.
            memset(endpointDesc, 0, sizeof(usb_ohci_endpoint_desc_t));
            endpointDesc->InUse = true;

            // If a previous descriptor was specified, link this one onto it.
            if (previousDesc != NULL)
                previousDesc->NextEndpointDesc = ((uint32_t)pmm_dma_get_phys((uintptr_t)endpointDesc));

            // No more descriptors after this. This descriptor can be specified later to change it.
            endpointDesc->NextEndpointDesc = 0;

            // Populate descriptor.
            endpointDesc->FunctionAddress = usbDevice->Address;
            endpointDesc->EndpointNumber = endpoint->Number;
            endpointDesc->Direction = 0x0; // Direction is up to transfer descriptors.
            endpointDesc->LowSpeed = usbDevice->Speed == USB_SPEED_LOW;
            endpointDesc->IsochronousFormat = endpoint->Type == USB_ENDPOINT_TRANSFERTYPE_ISO;
            endpointDesc->MaxPacketSize = endpoint->MaxPacketSize;
            endpointDesc->HeadPointer = (uint32_t)pmm_dma_get_phys((uintptr_t)headTransferDesc);

            // Return the descriptor.
            return endpointDesc;
        }
    }

    // No descriptor was found.
    panic("OHCI: No more available endpoint descriptors!\n");
}

static void usb_ohci_endpoint_desc_free(usb_ohci_endpoint_desc_t *endpointDesc) {
    // Mark descriptor as free.
    memset(endpointDesc, 0, sizeof(usb_ohci_endpoint_desc_t));
    endpointDesc->InUse = false;
}

static usb_ohci_transfer_desc_t *usb_ohci_transfer_desc_alloc(usb_ohci_controller_t *controller, usb_ohci_transfer_desc_t *previousDesc,
    uint8_t type, bool toggle, void* data, uint8_t packetSize) {
    // Search for first free transfer descriptor.
    for (usb_ohci_transfer_desc_t *transferDesc = controller->TransferDescPool;
        (uintptr_t)transferDesc + sizeof(usb_ohci_transfer_desc_t) < (uintptr_t)controller->TransferDescPool + USB_OHCI_TRANSFER_POOL_SIZE; transferDesc++) {
        if (!transferDesc->InUse) {
            // Zero out descriptor and mark in-use.
            memset(transferDesc, 0, sizeof(usb_ohci_transfer_desc_t));
            transferDesc->InUse = true;

            // If a previous descriptor was specified, link this one onto it.
            if (previousDesc != NULL) {
                previousDesc->NextTransferDesc = ((uint32_t)pmm_dma_get_phys((uintptr_t)transferDesc));
                previousDesc->Next = previousDesc->NextTransferDesc;
            }

            // No more descriptors after this. This descriptor can be specified later to change it.
            transferDesc->NextTransferDesc = 0;
            transferDesc->Next = 0;

            // Populate descriptor.
            transferDesc->BufferRounding = true; // Enable buffer rounding.
            transferDesc->Direction = type; // Type of packet (SETUP, IN, or OUT).
            transferDesc->DelayInterrupt = 0x7;
            transferDesc->DataToggle = toggle;
            transferDesc->NoToggleCarry = true;
            transferDesc->ConditionCode = USB_OHCI_TD_CONDITION_NOT_ACCESSED; // Per Table 4-7: Completion Codes.

            // If there is a buffer desired, add it.
            if (data != NULL) {
                transferDesc->CurrentBufferPointer = (uint32_t)pmm_dma_get_phys((uintptr_t)data); // Physical address of buffer.
                transferDesc->BufferEnd = transferDesc->CurrentBufferPointer + packetSize - 1;
            }

            // Return the descriptor.
            return transferDesc;
        }
    }

    // No descriptor was found.
    panic("OHCI: No more available transfer descriptors!\n");
}

static void usb_ohci_transfer_desc_free(usb_ohci_transfer_desc_t *transferDesc) {
    // Mark descriptor as free.
    memset(transferDesc, 0, sizeof(usb_ohci_transfer_desc_t));
    transferDesc->InUse = false;
}

static void usb_ohci_address_free(usb_device_t *usbDevice) {
    // Get the OHCI controller.
    usb_ohci_controller_t *controller = (usb_ohci_controller_t*)usbDevice->Controller;

    // Mark address free.
    controller->AddressPool[usbDevice->Address - 1] = false;
}

static bool usb_ohci_address_alloc(usb_device_t *usbDevice) {
    // Get the OHCI controller.
    usb_ohci_controller_t *controller = (usb_ohci_controller_t*)usbDevice->Controller;

    // Search for free address. Addresses start at 1 and end at 127.
    for (uint8_t i = 0; i < USB_MAX_DEVICES; i++) {
        if (!controller->AddressPool[i]) {
            // Build transfer to set address.
            usb_control_transfer_t transferSetAddress = { };
            transferSetAddress.Inbound = false;
            transferSetAddress.Type = USB_REQUEST_TYPE_STANDARD;
            transferSetAddress.Recipient = USB_REQUEST_REC_DEVICE;
            transferSetAddress.Request = USB_REQUEST_SET_ADDRESS;
            transferSetAddress.ValueLow = i + 1;
            transferSetAddress.ValueHigh = 0;
            transferSetAddress.Index = 0;

            // Set the address.
            if (!usbDevice->ControlTransfer(usbDevice, usbDevice->EndpointZero, transferSetAddress, NULL, 0))
                return false;

            // Free the old address, if any.
            if (usbDevice->Address)
                usb_ohci_address_free(usbDevice);

            // Mark address in-use and save to device object.
            controller->AddressPool[i] = true;
            usbDevice->Address = i + 1;
            return true;
        }
    }

    // No address found, too many USB devices attached to host most likely.
    return false;
}

static uint32_t usb_ohci_read(usb_ohci_controller_t *controller, uint8_t reg) {
    // Read the result from the OHCI controller's memory.
    return *(volatile uint32_t*)((uintptr_t)controller->BasePointer + reg);
}

static void usb_ohci_write(usb_ohci_controller_t *controller, uint8_t reg, uint32_t value) {
    // Write to the OHCI controller's memory.
    *(volatile uint32_t*)((uintptr_t)controller->BasePointer + reg) = value;
}

static bool usb_ohci_device_control(usb_device_t* device, usb_endpoint_t *endpoint, usb_control_transfer_t transfer, void *buffer, uint16_t length) {
    // Get the OHCI controller.
    usb_ohci_controller_t *controller = (usb_ohci_controller_t*)device->Controller;

    // Temporary buffer.
    void *usbBuffer = NULL;

    // Create USB request.
    usb_request_t *request = (usb_request_t*)usb_ohci_alloc(controller, sizeof(usb_request_t));
    memset(request, 0, sizeof(usb_request_t));
    request->Inbound = transfer.Inbound;
    request->Type = transfer.Type;
    request->Recipient = transfer.Recipient;
    request->Request = transfer.Request;
    request->ValueLow = transfer.ValueLow;
    request->ValueHigh = transfer.ValueHigh;
    request->Index = transfer.Index;
    request->Length = length;

    // Create SETUP transfer descriptor.
    usb_ohci_transfer_desc_t *transferDesc = usb_ohci_transfer_desc_alloc(controller, NULL, USB_OHCI_TD_PACKET_SETUP, false, request, sizeof(usb_request_t));
    usb_ohci_transfer_desc_t *headDesc = transferDesc;
    usb_ohci_transfer_desc_t *prevDesc = transferDesc;

    // Create endpoint descriptor with the SETUP transfer descriptor as the head.
    usb_ohci_endpoint_desc_t *endpointDesc = usb_ohci_endpoint_desc_alloc(controller, NULL, device, endpoint, headDesc);

    // Determine whether packets are to be IN or OUT.
    uint8_t packetType = transfer.Inbound ? USB_OHCI_TD_PACKET_IN : USB_OHCI_TD_PACKET_OUT;

    // Create packets for data.
    if (length > 0) {
        // Create temporary buffer that the UHCI controller can use.
        usbBuffer = usb_ohci_alloc(controller, length);
        if (!transfer.Inbound)
            memcpy(usbBuffer, buffer, length);

        uint8_t *packetPtr = (uint8_t*)usbBuffer;
        uint8_t *endPtr = packetPtr + length;
        bool toggle = false;
        while (packetPtr < endPtr) {
            // Toggle toggle bit.
            toggle = !toggle;

            // Determine size of packet, and cut it down if above max size.
            uint16_t remainingData = endPtr - packetPtr;
            uint8_t packetSize = remainingData > endpoint->MaxPacketSize ? endpoint->MaxPacketSize : remainingData;
            
            // Create packet.
            transferDesc = usb_ohci_transfer_desc_alloc(controller, prevDesc, packetType, toggle, packetPtr, packetSize);

            // Move to next packet.
            packetPtr += packetSize;
            prevDesc = transferDesc;
        }
    }

    // Create status packet.
    packetType = transfer.Inbound ? USB_OHCI_TD_PACKET_OUT : USB_OHCI_TD_PACKET_IN;
    transferDesc = usb_ohci_transfer_desc_alloc(controller, prevDesc, packetType, true, NULL, 0);

    // Add endpoint descriptor to OHCI control list. This requires disabling control transfers, adding the descriptor, and enabling them again.
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL, usb_ohci_read(controller, USB_OHCI_REG_CONTROL) & ~USB_OHCI_REG_CONTROL_CONTROL_ENABLE);
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL_HEAD_ED, (uint32_t)pmm_dma_get_phys((uintptr_t)endpointDesc));
    usb_ohci_write(controller, USB_OHCI_REG_COMMAND_STATUS, usb_ohci_read(controller, USB_OHCI_REG_COMMAND_STATUS) | USB_OHCI_REG_STATUS_CONTROL_LIST_FILLED);
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL, usb_ohci_read(controller, USB_OHCI_REG_CONTROL) | USB_OHCI_REG_CONTROL_CONTROL_ENABLE);

    // Wait for endpoint descriptor to complete.
    bool result = true;
    while (endpointDesc->HeadPointer != NULL)
    {
        // If a descriptor got itself halted, the transfer failed.
        if (endpointDesc->HeadPointer & USB_OHCI_DESC_HALTED) {
            result = false;
            break;
        }
    }

    // Disable processing of control list.
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL, usb_ohci_read(controller, USB_OHCI_REG_CONTROL) & ~USB_OHCI_REG_CONTROL_CONTROL_ENABLE);
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL_HEAD_ED, 0);

    // Free up endpoint descriptor.
    usb_ohci_endpoint_desc_free(endpointDesc);

    // Free up transfer descriptors.
    transferDesc = headDesc;
    usb_ohci_transfer_desc_t *nextDesc;
    while (transferDesc != NULL) {
        // Get next descriptor if it exists.
        if (transferDesc->Next)
            nextDesc = (usb_ohci_transfer_desc_t*)(pmm_dma_get_virtual(transferDesc->Next));
        else
            nextDesc = NULL;

        // Free transfer descriptor and move to next.
        usb_ohci_transfer_desc_free(transferDesc);
        transferDesc = nextDesc;
    }

    // Copy data if the operation succeeded.
    if (transfer.Inbound && usbBuffer != NULL && result)
        memcpy(buffer, usbBuffer, length);

    // Free buffers.
    usb_ohci_free(controller, request, sizeof(usb_request_t));
    if (usbBuffer != NULL)
        usb_ohci_free(controller, usbBuffer, length);

    return result;
}

static void usb_ohci_device_interrupt_start(usb_device_t *device, usb_endpoint_t *endpoint, uint16_t length) {
    // Get the OHCI controller.
    usb_ohci_controller_t *controller = (usb_ohci_controller_t*)device->Controller;

    // Temporary buffer.
    void *usbBuffer = usb_ohci_alloc(controller, length);

    // Create SETUP transfer descriptor.
    usb_ohci_transfer_desc_t *transferDesc = usb_ohci_transfer_desc_alloc(controller, NULL, USB_OHCI_TD_PACKET_IN, false, usbBuffer, length);

    // Create endpoint descriptor with the SETUP transfer descriptor as the head.
    usb_ohci_endpoint_desc_t *endpointDesc = usb_ohci_endpoint_desc_alloc(controller, NULL, device, endpoint, transferDesc);

    for (int i = 0; i < 32; i++)
        controller->Hcca->InterruptTable[i] = (uint32_t)pmm_dma_get_phys((uintptr_t)endpointDesc);
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL, usb_ohci_read(controller, USB_OHCI_REG_CONTROL) | USB_OHCI_REG_CONTROL_PERIOD_ENABLE);

    while(true) {
       // kprintf("");
       sleep(100);
        kprintf("st 0x%X\n", usb_ohci_read(controller, USB_OHCI_REG_INTERRUPT_STATUS));
        if (transferDesc->ConditionCode != 0xF) {
            kprintf("");
        }
    }
}

static void usb_ohci_reset(usb_ohci_controller_t *controller) {
    // Save content of frame interval register.
    uint32_t frameIntervalReg = usb_ohci_read(controller, USB_OHCI_REG_FRAME_INTERVAL);

    // Reset controller.
    usb_ohci_write(controller, USB_OHCI_REG_COMMAND_STATUS, USB_OHCI_REG_STATUS_RESET);
    while (usb_ohci_read(controller, USB_OHCI_REG_COMMAND_STATUS) & USB_OHCI_REG_STATUS_RESET);
    
    // Restore frame interval.
    usb_ohci_write(controller, USB_OHCI_REG_FRAME_INTERVAL, frameIntervalReg);

    // Set HCCA address.
    usb_ohci_write(controller, USB_OHCI_REG_HCCA, pmm_dma_get_phys((uintptr_t)controller->Hcca));

    // Set periodic start to 90% of frame interval.
    uint16_t frameInterval = (uint16_t)(usb_ohci_read(controller, USB_OHCI_REG_FRAME_INTERVAL) & 0x3FFF);
    usb_ohci_write(controller, USB_OHCI_REG_PERIODIC_START, frameInterval - (frameInterval / 10));

    // Enable controller.
    usb_ohci_write(controller, USB_OHCI_REG_CONTROL, (usb_ohci_read(controller, USB_OHCI_REG_CONTROL) & ~USB_OHCI_REG_CONTROL_STATE_USBSUSPEND) | USB_OHCI_REG_CONTROL_STATE_USBOPERATIONAL);
    uint32_t control = usb_ohci_read(controller, USB_OHCI_REG_CONTROL);
    sleep(200);
}

static uint32_t usb_ohci_reset_port(usb_ohci_controller_t *controller, uint8_t port) {
    uint32_t portReg = USB_OHCI_REG_RH_PORT_STATUS + (port - 1);

    // Get port status.
    uint32_t portStatus = usb_ohci_read(controller, portReg);

    // If port is not connected, just return 0.
    if (!(portStatus & USB_OHCI_PORT_STATUS_CONNECTED))
        return 0;

    // Clear status change bits.
    portStatus |= (USB_OHCI_PORT_STATUS_CONNECT_CHANGED | USB_OHCI_PORT_STATUS_ENABLE_CHANGED
        | USB_OHCI_PORT_STATUS_SUSPEND_CHANGED | USB_OHCI_PORT_STATUS_OVERCURRENT_CHANGED | USB_OHCI_PORT_STATUS_RESET_CHANGED);
    usb_ohci_write(controller, portReg, portStatus);

    // Reset port.
    usb_ohci_write(controller, portReg, usb_ohci_read(controller, portReg) | USB_OHCI_PORT_STATUS_RESET);

    // Wait for port to reset.
    for (uint8_t i = 0; i < 10; i++) {
        if (usb_ohci_read(controller, portReg) & USB_OHCI_PORT_STATUS_RESET)
            break;
        sleep(5);
    }
    portStatus = usb_ohci_read(controller, portReg);

    // Clear status change bits.
    portStatus |= (USB_OHCI_PORT_STATUS_CONNECT_CHANGED | USB_OHCI_PORT_STATUS_ENABLE_CHANGED
        | USB_OHCI_PORT_STATUS_SUSPEND_CHANGED | USB_OHCI_PORT_STATUS_OVERCURRENT_CHANGED | USB_OHCI_PORT_STATUS_RESET_CHANGED);
    usb_ohci_write(controller, portReg, portStatus);
    sleep(50);

    // Return status.
    return usb_ohci_read(controller, portReg);
}

static bool usb_ohci_probe(usb_ohci_controller_t *controller) {
    // Get characteristics of root hub.
    uint32_t rootHubRegA = usb_ohci_read(controller, USB_OHCI_REG_RH_DESCRIPTOR_A);
    uint32_t rootHubRegB = usb_ohci_read(controller, USB_OHCI_REG_RH_DESCRIPTOR_B);

    // Get number of ports.
    controller->NumPorts = USB_OHCI_REG_RH_GET_NUMPORTS(rootHubRegA);
    kprintf("OHCI: Controller has %u ports total.\n", controller->NumPorts);

    // Disable power switching and turn all ports on.
    rootHubRegA |= USB_OHCI_REG_RH_NO_POWER_SWITCHING;
    usb_ohci_write(controller, USB_OHCI_REG_RH_DESCRIPTOR_A, rootHubRegA);
    rootHubRegA = usb_ohci_read(controller, USB_OHCI_REG_RH_DESCRIPTOR_A);

    // Disable all ports.
    for (uint8_t port = 1; port <= 15; port++) {
        usb_ohci_write(controller, USB_OHCI_REG_RH_PORT_STATUS + (port - 1), USB_OHCI_PORT_STATUS_DISABLE);
    }

    for (uint8_t port = 1; port <= 15; port++) {
        uint32_t portStatus = usb_ohci_reset_port(controller, port);
        if (portStatus & USB_OHCI_PORT_STATUS_CONNECTED && portStatus & USB_OHCI_PORT_STATUS_ENABLED) {
            uint32_t control = usb_ohci_read(controller, USB_OHCI_REG_CONTROL);

            kprintf("OHCI: Port %u is connected at %s speed (0x%X)!\n", port, portStatus & USB_OHCI_PORT_STATUS_LOWSPEED ? "low" : "full", portStatus);
            usb_device_t *usbDevice = usb_device_create(controller->RootDevice, port, (portStatus & USB_OHCI_PORT_STATUS_LOWSPEED) ? USB_SPEED_LOW : USB_SPEED_FULL);
            if (usbDevice != NULL) {
                // Add as our child.
                if (controller->RootDevice->Children != NULL) {
                    usb_device_t *lastDevice = controller->RootDevice->Children;
                    while (lastDevice->Next != NULL)
                        lastDevice = lastDevice->Next;
                    lastDevice->Next = usbDevice;
                }
                else {
                    controller->RootDevice->Children = usbDevice;
                }
            }
        }
    }
}

void usb_ohci_init(pci_device_t *pciDevice) {
    kprintf("OHCI: Initializing...\n");

    // Create controller object and map to memory.
    usb_ohci_controller_t *controller = (usb_ohci_controller_t*)kheap_alloc(sizeof(usb_ohci_controller_t));
    memset(controller, 0, sizeof(usb_ohci_controller_t));
    controller->BaseAddress = pciDevice->BaseAddresses[0].BaseAddress;
    controller->BasePointer = (uint32_t*)paging_device_alloc(controller->BaseAddress, controller->BaseAddress);
    kprintf("OHCI: Mapping controller at 0x%X to 0x%p...\n", controller->BaseAddress, controller->BasePointer);

    // Get version and ensure we can support it.
    uint32_t version = usb_ohci_read(controller, USB_OHCI_REG_REVISION) & USB_OHCI_REG_REVISION_MASK;
    if (version < USB_OHCI_MIN_VERSION) {
        kprintf("OHCI: Controller version is too low, aborting! (%u.%u)\n", version >> 4, version & 0xF);
        kheap_free(controller);
        return;
    }
    kprintf("OHCI: Controller version: %u.%u\n", version >> 4, version & 0xF);

    // Get DMA frame to store various structures.
    uintptr_t frame;
    if (!pmm_dma_get_free_frame(&frame))
        panic("OHCI: Couldn't get DMA frame!\n");
    memset((void*)frame, 0, PAGE_SIZE_64K);
    controller->EndpointDescPool = (usb_ohci_endpoint_desc_t*)frame;
    controller->TransferDescPool = (usb_ohci_transfer_desc_t*)((uintptr_t)controller->EndpointDescPool + USB_OHCI_ENDPOINT_POOL_SIZE);
    controller->HeapPool = (uint8_t*)((uintptr_t)controller->TransferDescPool + USB_OHCI_TRANSFER_POOL_SIZE);

    // Create root USB device.
    controller->RootDevice = (usb_device_t*)kheap_alloc(sizeof(usb_device_t));
    memset(controller->RootDevice, 0, sizeof(usb_device_t));
    if (controller->RootDevice == NULL) {
        kprintf("OHCI: Failed to create root device!\n");
        kheap_free(controller);
        return;
    }

    // No parent means it is on the root hub.
    controller->RootDevice->Parent = NULL;
    controller->RootDevice->Controller = controller;
    controller->RootDevice->AllocAddress = usb_ohci_address_alloc;
    controller->RootDevice->FreeAddress = usb_ohci_address_free;
    controller->RootDevice->ControlTransfer = usb_ohci_device_control;
    controller->RootDevice->InterruptTransferStart = usb_ohci_device_interrupt_start;

    // These fields are mostly meaningless as this is the root hub.
    controller->RootDevice->Port = 0;
    controller->RootDevice->Speed = USB_SPEED_FULL;
    controller->RootDevice->Address = 255;
    controller->RootDevice->VendorId = pciDevice->VendorId;
    controller->RootDevice->ProductId = pciDevice->DeviceId;
    controller->RootDevice->Class = USB_CLASS_HUB;
    controller->RootDevice->Subclass = 0;
    controller->RootDevice->Protocol = 0;
    controller->RootDevice->VendorString = "SydOS";
    controller->RootDevice->ProductString = "OHCI Root Hub";
    controller->RootDevice->SerialNumber = USB_SERIAL_GENERIC;

    controller->RootDevice->NumConfigurations = 0;
    controller->RootDevice->CurrentConfigurationValue = 0;
    controller->RootDevice->Configured = true;
    if (StartUsbDevice != NULL) {
        usb_device_t *lastDevice = StartUsbDevice;
        while (lastDevice->Next != NULL)
            lastDevice = lastDevice->Next;
        lastDevice->Next = controller->RootDevice;
    }
    else
        StartUsbDevice = controller->RootDevice;

    // Initialize host controller area.
    controller->Hcca = (usb_ohci_controller_comm_area_t*)usb_ohci_alloc(controller, sizeof(usb_ohci_controller_comm_area_t));
    memset(controller->Hcca, 0, sizeof(usb_ohci_controller_comm_area_t));

    // Get ownership.
    uint32_t control = usb_ohci_read(controller, USB_OHCI_REG_CONTROL);
    if (control & USB_OHCI_REG_CONTROL_INTERRUPT_ROUTING) {
        kprintf("OHCI: Taking ownership for controller...\n");
        usb_ohci_write(controller, USB_OHCI_REG_COMMAND_STATUS, USB_OHCI_REG_STATUS_OWNERSHIP_CHANGE_REQ);
        while (usb_ohci_read(controller, USB_OHCI_REG_CONTROL) & USB_OHCI_REG_CONTROL_INTERRUPT_ROUTING);
        control = usb_ohci_read(controller, USB_OHCI_REG_CONTROL);
        kprintf("OHCI: Ownership acquired!\n");
    }

    // Reset the controller.
    usb_ohci_reset(controller);
    control = usb_ohci_read(controller, USB_OHCI_REG_CONTROL);

    // Probe.
    usb_ohci_probe(controller);


}
