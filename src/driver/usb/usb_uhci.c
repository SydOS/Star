#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_uhci.h>

#include <driver/usb/usb_descriptors.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_device.h>

#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

static usb_uhci_transfer_desc_t *usb_uhci_transfer_desc_alloc(usb_uhci_controller_t *controller) {
    // Search for first free transfer descriptor.
    for (usb_uhci_transfer_desc_t *transferDesc = controller->TransferDescPool;
        ((uintptr_t)transferDesc + sizeof(usb_uhci_transfer_desc_t)) <= (controller->TransferDescPool + USB_UHCI_TD_POOL_SIZE); transferDesc++) {
        if (!transferDesc->InUse) {
            // Mark descriptor as active and return it.
            transferDesc->InUse = true;
            return transferDesc;
        }
    }

    // No descriptor was found.
    panic("UHCI: No more available transfer descriptors!\n");
}

static void usb_uhci_transfer_desc_free(usb_uhci_transfer_desc_t *transferDesc) {
    // Mark descriptor as free.
    transferDesc->InUse = false;
}

static usb_uhci_queue_head_t *usb_uhci_queue_head_alloc(usb_uhci_controller_t *controller) {
    // Search for first free queue head.
    for (usb_uhci_queue_head_t *queueHead = controller->QueueHeadPool;
        ((uintptr_t)queueHead + sizeof(usb_uhci_queue_head_t)) <= (controller->QueueHeadPool + USB_UHCI_QH_POOL_SIZE); queueHead++) {
        if (!queueHead->InUse) {
            // Mark queue head as active and return it.
            queueHead->InUse = true;
            return queueHead;
        }
    }

    // No queue head was found.
    panic("UHCI: No more available queue heads!\n");
}

static void usb_uhci_queue_head_free(usb_uhci_queue_head_t *queueHead) {
    // Mark queue head as free.
    queueHead->InUse = false;
}

static void usb_uhci_queue_head_add(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead) {
    usb_uhci_queue_head_t *end = (usb_uhci_queue_head_t*)pmm_dma_get_virtual(controller->QueueHead->PreviousQueueHead);  //(usb_uhci_queue_head_t*)pmm_dma_get_virtual(((uint8_t*)controller->QueueHead->PreviousQueueHead) - (uint32_t)(&(((usb_uhci_queue_head_t*)0)->PreviousQueueHead)));

    uint16_t d = inw(USB_UHCI_FRNUM(controller->BaseAddress));
    queueHead->Head = USB_UHCI_FRAME_TERMINATE;
    end->Head = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead) | USB_UHCI_FRAME_QUEUE_HEAD;

    usb_uhci_queue_head_t *n = (usb_uhci_queue_head_t*)pmm_dma_get_virtual(controller->QueueHead->NextQueueHead);
    n->PreviousQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead);
    queueHead->NextQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)n);
    queueHead->PreviousQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)controller->QueueHead);
    controller->QueueHead->NextQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead);
   // queueHead->NextQueueHead->PreviousQueueHead = end;
}

static bool usb_uhci_queue_head_process(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead, bool *outStatus) {
    bool complete = false;

    // Get transfer descriptor (last 4 bits are control, so ignore them).
    usb_uhci_transfer_desc_t *transferDesc = (usb_uhci_transfer_desc_t*)pmm_dma_get_virtual(queueHead->Element & ~0xF);
    uint16_t d = inw(USB_UHCI_FRNUM(controller->BaseAddress));
    uint16_t s = inw(USB_UHCI_USBSTS(controller->BaseAddress));
    
    uint16_t pciS = pci_config_read_word(controller->PciDevice, PCI_REG_STATUS);
    
    uint32_t* t1 = (uint32_t*)transferDesc;
    uint32_t* t2 = ((uint32_t*)transferDesc)+1;
    uint32_t* t3 = ((uint32_t*)transferDesc)+2;
    uint32_t* t4 = ((uint32_t*)transferDesc)+3;

    // If no transfer descriptor, we are done.
    if (transferDesc == NULL) {
        complete = true;
        *outStatus = true;
    }
    // Is the transfer descriptor inactive?
    else if(!(transferDesc->Active)) {
        // Is the transfer stalled?
        if (transferDesc->Stalled) {
            kprintf("UHCI: stall:\n");
            complete = true;
            outStatus = false;
        }
        if (transferDesc->DataBufferError)
            kprintf("UHCI: data buffer error\n");
        if (transferDesc->BabbleDetected)
            kprintf("UHCI: babble\n");
        if (transferDesc->NakReceived)
            kprintf("UHCI: nak\n");
        if (transferDesc->CrcError)
            kprintf("UHCI: crc error\n");
        if (transferDesc->BitstuffError)
            kprintf("UHCI: bitstuff\n");
        kprintf("UHCI: packet type: 0x%X\n", transferDesc->PacketIdentification);
        kprintf("UHCI: packet pointer: 0x%X\n", transferDesc->BufferPointer);
    }

    // Are we done with transfers?
    if (complete) {
        // Remove queue head from schedule.
        kprintf("done!\n");
        kprintf("status: 0x%X\n", s);
        kprintf("pci status: 0x%X\n", pciS);
        kprintf("usb cmd: 0x%X\n", inw(USB_UHCI_USBCMD(controller->BaseAddress)));
        kprintf("port 1: 0x%X\n", inw(USB_UHCI_PORTSC1(controller->BaseAddress)));

        //uint64_t *d = (uint64_t*)pmm_dma_get_virtual(transferDesc->BufferPointer);
        //kprintf_nlock("0x%X\n", *d);

       // while(true);

        // Free transfer descriptors.
        transferDesc = (usb_uhci_transfer_desc_t*)pmm_dma_get_virtual(queueHead->TransferDescHead);
        while (transferDesc != NULL) {
            // Free transcript descriptor.
            usb_uhci_transfer_desc_t* nextDesc = (usb_uhci_transfer_desc_t*)pmm_dma_get_virtual(transferDesc->NextDesc);
            usb_uhci_transfer_desc_free(transferDesc);
            transferDesc = nextDesc;
        }

        // Free queue head.
        usb_uhci_queue_head_free(queueHead);
    }
    return complete;
}

static void usb_uhci_queue_head_wait(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead) {
    // Wait for all queue heads to be processed.
    bool status;
    while (!usb_uhci_queue_head_process(controller, queueHead, &status));
}

static void usb_uhci_transfer_desc_init(usb_uhci_transfer_desc_t *desc, usb_uhci_transfer_desc_t *previous,
    bool lowSpeed, uint8_t address, uint8_t endpoint, bool dataToggle, uint8_t packetType, uint16_t length, const void *data) {
    // If a previous descriptor was specified, link this one onto it.
    if (previous != NULL)
        previous->LinkPointer = ((uint32_t)pmm_dma_get_phys((uintptr_t)desc)) | USB_UHCI_FRAME_DEPTH_FIRST;

    // No more descriptors after this.
    desc->LinkPointer = USB_UHCI_FRAME_TERMINATE;

    // Set speed and error count (3 errors before stalled).
    desc->LowSpeedDevice = lowSpeed;
    desc->Active = true;
    desc->ErrorCounter = 3;

    // Adjust length field to account for a null packet (0 bytes in size). 0x7FF indicates a null packet.
    // 0 = 1 bytes, 1 = 2 bytes, etc.
    length = (length - 1) & 0x7FF;

    // Set token fields.
    desc->MaximumLength = length;
    desc->DataToggle = dataToggle;
    desc->Endpoint = endpoint;
    desc->DeviceAddress = address;
    desc->PacketIdentification = packetType;

    // Set buffer pointer.
    desc->BufferPointer = (uint32_t)pmm_dma_get_phys((uintptr_t)data);
}

static bool usb_uhci_write_device(usb_device_t *device, uint8_t endpoint, usb_request_t *request, void *data, uint16_t length) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)device->Controller;

    // Get device attributes.
    bool lowSpeed = device->Speed == USB_SPEED_LOW;
    uint8_t address = device->Address;
    uint8_t maxPacketSize = device->MaxPacketSize;

    // Create first transfer descriptor.
    usb_uhci_transfer_desc_t *transferDesc = usb_uhci_transfer_desc_alloc(controller);
    usb_uhci_transfer_desc_t *headDesc = transferDesc;
    usb_uhci_transfer_desc_t *prevDesc = NULL;

    // Create SETUP packet.
    usb_uhci_transfer_desc_init(transferDesc, NULL, lowSpeed, address, endpoint, false, USB_UHCI_TD_PACKET_SETUP, sizeof(usb_request_t), request);
    prevDesc = transferDesc;

    // Determine whether packets are to be IN or OUT.
    uint8_t packetType = request->Type & USB_REQUEST_TYPE_DEVICE_TO_HOST ? USB_UHCI_TD_PACKET_IN : USB_UHCI_TD_PACKET_OUT;

    // Create packets for data.
    uint8_t *packetPtr = (uint8_t*)data;
    uint8_t *endPtr = packetPtr + length;
    bool toggle = false;
    while (packetPtr < endPtr) {
        // Allocate a transfer descriptor for packet.
        transferDesc = usb_uhci_transfer_desc_alloc(controller);

        // Toggle toggle bit.
        toggle = !toggle;

        // Determine size of packet, and cut it down if above max size.
        uint8_t packetSize = endPtr - packetPtr;
        if (packetSize > maxPacketSize)
            packetSize = maxPacketSize;
        
        // Create packet.
        usb_uhci_transfer_desc_init(transferDesc, prevDesc, lowSpeed, address, endpoint, toggle, packetType, packetSize, packetPtr);

        // Move to next packet.
        packetPtr += packetSize;
        prevDesc = transferDesc;
    }

    // Create status packet.
    transferDesc = usb_uhci_transfer_desc_alloc(controller);
    packetType = request->Type & USB_REQUEST_TYPE_DEVICE_TO_HOST ? USB_UHCI_TD_PACKET_OUT : USB_UHCI_TD_PACKET_IN;
    usb_uhci_transfer_desc_init(transferDesc, prevDesc, lowSpeed, address, endpoint, true, packetType, 0, NULL);

    // Create queue head that starts with first transfer descriptor.
    usb_uhci_queue_head_t *queueHead = usb_uhci_queue_head_alloc(controller);
    queueHead->TransferDescHead = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);
    queueHead->Element = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);

    // Add queue head to schedule and wait for completion.
    usb_uhci_queue_head_add(controller, queueHead);
    usb_uhci_queue_head_wait(controller, queueHead);
}

/*static void usb_uhci_write_port(uint16_t port, uint16_t data, bool clear) {
    // Get current port status/control.
    uint16_t status = inw(port);

    if (!clear)
        status |= data;

    // Add data to port and pull out two changed bits.
    status &= ~(USB_UHCI_PORT_STATUS_PRESENT_CHANGE | USB_UHCI_PORT_STATUS_ENABLE_CHANGE);

    // Clear the changed bits if specified.
    if (clear) {
        status &= ~data;
        status |= (USB_UHCI_PORT_STATUS_PRESENT_CHANGE | USB_UHCI_PORT_STATUS_ENABLE_CHANGE) & data;
    }

    // Write result to port.
    outw(port, status);
}*/

static uint16_t usb_uhci_reset_port(usb_uhci_controller_t *controller, uint8_t port) {
    // Determine port register.
    uint16_t portReg = USB_UHCI_PORTSC1(controller->BaseAddress) + (port * sizeof(uint16_t));

    // Reset port and wait.
    sleep(100);
    uint16_t status = inw(portReg);
    status &= USB_UHCI_PORTSC_MASK;
    outw(portReg, status | USB_UHCI_PORTSC_RESET);
    sleep(50);

    // Clear reset bit.
    status = inw(portReg);
    status &= USB_UHCI_PORTSC_MASK;
    outw(portReg, status & ~USB_UHCI_PORTSC_RESET);
    sleep(100);

    // Reset port.
    //usb_uhci_write_port(portReg, USB_UHCI_PORT_STATUS_RESET, false);
  //  sleep(58);
  //  usb_uhci_write_port(portReg, USB_UHCI_PORT_STATUS_RESET, true);
  //  sleep(1000);

    // Wait for port to be connected and enabled.
    for (uint16_t i = 0; i < 100; i++) {
        // Get the current status of the port.
        status = inw(portReg);

        // Ensure a device is attached. If not, abort.
        if (~status & USB_UHCI_PORTSC_PRESENT)
            break;

        // Clear any changed bits.
        if (status & (USB_UHCI_PORTSC_PRESENT_CHANGE | USB_UHCI_PORTSC_ENABLE_CHANGE)) {
            outw(portReg, status);
            continue;
        }

        // If the port is enabled, we are done. Otherwise enable the port and check again.
        if (status & USB_UHCI_PORTSC_ENABLED)
            break;

        // Get the current status of the port.
        //status = inw(portReg);
        status &= USB_UHCI_PORTSC_MASK;

        // Enable port.
        outw(portReg, status | USB_UHCI_PORTSC_ENABLED);
        sleep(50);
    }

    // Return the status.
    return status;
}

static void usb_uhci_reset_global(usb_uhci_controller_t *controller) {
    // Save SOF register.
    uint8_t sof = inb(USB_UHCI_SOFMOD(controller->BaseAddress));

    // Perform global reset.
    uint16_t status = inw(USB_UHCI_USBCMD(controller->BaseAddress));
    status |= USB_UHCI_STATUS_GLOBAL_RESET;
    outw(USB_UHCI_USBCMD(controller->BaseAddress), status);

    // Wait 10ms.
    sleep(10);

    // Clear bit.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), inw(USB_UHCI_USBCMD(controller->BaseAddress)) & ~USB_UHCI_STATUS_GLOBAL_RESET);
    sleep(10);

    // Restore SOF.
    outb(USB_UHCI_SOFMOD(controller->BaseAddress), sof);
}

static bool usb_uhci_reset(usb_uhci_controller_t *controller) {
    // Clear run bit.
    uint16_t status = inw(USB_UHCI_USBCMD(controller->BaseAddress));
    status &= ~USB_UHCI_STATUS_RUN;
    outw(USB_UHCI_USBCMD(controller->BaseAddress), status);

    // Wait for stop.
    while ((inw(USB_UHCI_USBSTS(controller->BaseAddress)) & 0x20) == 0) {
        kprintf("UHCI: Waiting for controller to stop...\n");
        sleep(10);
    }

    // Clear configure bit.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), inw(USB_UHCI_USBCMD(controller->BaseAddress)) & ~USB_UHCI_STATUS_CONFIGURE);

    // Reset controller.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RESET);

    // Wait for controller to reset.
    for (uint16_t i = 0; i < 100; i++) {
        sleep(100);

        // Check if controller reset bit is clear.
        if (!(inw(USB_UHCI_USBCMD(controller->BaseAddress)) & USB_UHCI_STATUS_RESET))
            return true;
    }
    return false;
}

static void usb_uhci_change_state(usb_uhci_controller_t *controller, bool run) {
    // Get contents of status register.
    uint16_t status = inw(USB_UHCI_USBCMD(controller->BaseAddress));
    
    // Start or stop controller.
    status |= USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS;
    if (run)
        status |= USB_UHCI_STATUS_RUN;
    else
        status &= ~USB_UHCI_STATUS_RUN;

    // Write to status register.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), status);
}

void usb_uhci_init(PciDevice *device) {
    kprintf("UHCI: Initializing...\n");
    // Create UHCI controller object.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)kheap_alloc(sizeof(usb_uhci_controller_t));
    device->DriverObject = controller;

    // Populate UHCI object.
    controller->PciDevice = device;
    controller->BaseAddress = (device->BAR[4] & PCI_BAR_PORT_MASK);
    controller->SpecVersion = pci_config_read_byte(device, USB_UHCI_PCI_REG_RELEASE_NUM);
    kprintf("UHCI: Controller located at 0x%X, version 0x%X.\n", controller->BaseAddress, controller->SpecVersion);

    outw(USB_UHCI_USBCMD(controller->BaseAddress), 0);
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0);

    // Take ownership.
    kprintf("UHCI: Taking ownership from BIOS...\n");
    uint16_t ownership = pci_config_read_word(device, 0xC0);
    kprintf("UHCI: Current value: 0x%X\n", ownership);
    pci_config_write_word(device, 0xC0, 0x8F00);
    kprintf("UHCI: SBRN 0x%X\n", pci_config_read_word(device, 0x60));

    // bus master
	uint16_t cmd = pci_config_read_word(device, PCI_REG_COMMAND);
    kprintf("UHCI: PCI command 0x%X\n", cmd);
	//pci_config_write_word(device, PCI_REG_COMMAND, cmd | 0x04);
	//cmd = pci_config_read_word(device, PCI_REG_COMMAND);
    
    // Reset controller.
    kprintf("UHCI: Performing global reset...\n");
    usb_uhci_reset_global(controller);
    kprintf("UHCI: Resetting controller...\n");
    if (!usb_uhci_reset(controller)) {
        kprintf("UHCI: Failed to reset controller! Aborting.\n");
        return;
    }
    sleep(100);
    outw(USB_UHCI_USBCMD(controller->BaseAddress), 0);
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0);

    // Pull a DMA frame for USB frame storage.
    if (!pmm_dma_get_free_frame(&controller->FrameList))
        panic("UHCI: Couldn't get DMA frame!\n");
    memset(controller->FrameList, 0, PAGE_SIZE_64K);

    // Get pointers to frame list and pools.
    kprintf("UHCI: Frame list located at: 0x%p (0x%X)\n", controller->FrameList, (uint32_t)pmm_dma_get_phys(controller->FrameList));
    controller->TransferDescPool = (usb_uhci_transfer_desc_t*)((uintptr_t)controller->FrameList + (USB_UHCI_FRAME_COUNT * sizeof(uint32_t)));
    controller->QueueHeadPool = (usb_uhci_queue_head_t*)((uintptr_t)controller->TransferDescPool + USB_UHCI_TD_POOL_SIZE);

    // Set frame base address and frame number.
    outl(USB_UHCI_FRBASEADD(controller->BaseAddress), (uint32_t)pmm_dma_get_phys(controller->FrameList));
    outw(USB_UHCI_FRNUM(controller->BaseAddress), 0);

    // Set max packet size to 64 bytes.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), inw(USB_UHCI_USBCMD(controller->BaseAddress)) | USB_UHCI_STATUS_64_BYTE_PACKETS);
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0);

    // Setup frame list.
    usb_uhci_queue_head_t *queueHead = usb_uhci_queue_head_alloc(controller);
    queueHead->Head = USB_UHCI_FRAME_TERMINATE;
    queueHead->Element = USB_UHCI_FRAME_TERMINATE;
    queueHead->PreviousQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead);
    queueHead->NextQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead);
    controller->QueueHead = queueHead;

    // Fill frame list.
    for (uint16_t i = 0; i < USB_UHCI_FRAME_COUNT; i++)
        controller->FrameList[i] = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead) | USB_UHCI_FRAME_QUEUE_HEAD;
  //  outb(USB_UHCI_SOFMOD(controller->BaseAddress), 0x40);

    // Start up controller.
    usb_uhci_change_state(controller, true);

    // Query root ports.
    for (uint16_t port = 0; port < 2; port++) {
        // Reset the port and get its status.
        uint16_t portStatus = usb_uhci_reset_port(controller, port);
        kprintf("UHCI: Port status for port %u: 0x%X\n", port, portStatus);

        // Is the port enabled?
        if (portStatus & USB_UHCI_PORTSC_ENABLED) {
            kprintf("UHCI: Port %u is enabled, at %s speed!\n", port + 1, (portStatus & USB_UHCI_PORTSC_LOW_SPEED) ? "low" : "full");

            // Create a USB device on the port.
            usb_device_t *usbDevice = usb_device_create();
            if (usbDevice != NULL) {
                // No parent means it is on the root hub.
                usbDevice->Parent = NULL;
                usbDevice->Controller = controller;

                // Set port and speed.
                usbDevice->Port = port;
                usbDevice->Speed = (portStatus & USB_UHCI_PORTSC_LOW_SPEED);
                usbDevice->MaxPacketSize = 8;

                usb_request_t *req = (usb_request_t*)((uintptr_t)controller->QueueHeadPool + USB_UHCI_QH_POOL_SIZE);
                memset(req, 0, sizeof(usb_request_t));
                req->Type = USB_REQUEST_TYPE_DEVICE_TO_HOST | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TYPE_REC_DEVICE;
                req->Request = USB_REQUEST_GET_DESCRIPTOR;
                req->Value = 0x1 << 8;
                req->Index = 0;
                req->Length = sizeof(usb_descriptor_device_t);
                uint64_t *g = (uint64_t*)&req;
                
                usb_descriptor_device_t *desc = (usb_request_t*)((uintptr_t)req + sizeof(usb_request_t));
                memset(desc, 0, sizeof(usb_descriptor_device_t));
                sleep(1000);
                usb_uhci_write_device(usbDevice, 0, req, desc, sizeof(usb_descriptor_device_t));
                kprintf("Vendor: 0x%X\n", desc->VendorId);
                while(true);
            }
        }
        else {
            kprintf("UHCI: Port %u is disabled!\n", port + 1);
        }
    }
}