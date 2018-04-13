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

static usb_uhci_transfer_desc_t *usb_uhci_transfer_desc_alloc(usb_uhci_controller_t *controller, usb_device_t *usbDevice,
    usb_uhci_transfer_desc_t *previousDesc, uint8_t type, uint8_t endpoint, bool toggle, void* data, uint8_t packetSize) {
    // Search for first free transfer descriptor.
    for (uint16_t i = 0; i < USB_UHCI_TD_POOL_COUNT; i++) {
        if (!controller->TransferDescMap[i]) {
            // Zero out descriptor and mark in-use.
            usb_uhci_transfer_desc_t *transferDesc = controller->TransferDescPool + i;
            memset(transferDesc, 0, sizeof(usb_uhci_transfer_desc_t));
            controller->TransferDescMap[i] = true;

            // If a previous descriptor was specified, link this one onto it.
            if (previousDesc != NULL)
                previousDesc->LinkPointer = ((uint32_t)pmm_dma_get_phys((uintptr_t)transferDesc)) | USB_UHCI_FRAME_DEPTH_FIRST;

            // No more descriptors after this. This descriptor can be specified later to change it.
            transferDesc->LinkPointer = USB_UHCI_FRAME_TERMINATE;

            // Populate descriptor.
            transferDesc->ErrorCounter = 3; // 3 errors before stall.
            transferDesc->LowSpeedDevice = usbDevice->Speed == USB_SPEED_LOW; // Low speed or full speed.
            transferDesc->Active = true; // Descriptor is active and will be executed.
            transferDesc->PacketType = type; // Type of packet (SETUP, IN, or OUT).
            transferDesc->DataToggle = toggle; // Data toggle for syncing.
            transferDesc->DeviceAddress = usbDevice->Address; // Address of device.
            transferDesc->Endpoint = endpoint; // Endpoint.
            transferDesc->MaximumLength = (packetSize - 1) & 0x7FF; // Length of packet. 0-length packet = 0x7FF.
            transferDesc->BufferPointer = (uint32_t)pmm_dma_get_phys((uintptr_t)data); // Physical address of buffer.

            // Return the descriptor.
            return transferDesc;
        }
    }

    // No descriptor was found.
    panic("UHCI: No more available transfer descriptors!\n");
}

static void usb_uhci_transfer_desc_free(usb_uhci_controller_t *controller, usb_uhci_transfer_desc_t *transferDesc) {
    // Mark descriptor as free.
    uint32_t index = ((uintptr_t)transferDesc - (uintptr_t)controller->TransferDescPool) / sizeof(usb_uhci_transfer_desc_t);
    controller->TransferDescMap[index] = false;
}

static usb_uhci_queue_head_t *usb_uhci_queue_head_alloc(usb_uhci_controller_t *controller) {
    // Search for first free queue head.
    for (uint16_t i = 0; i < USB_UHCI_QH_POOL_COUNT; i++) {
        if (!controller->QueueHeadMap[i]) {
            // Mark queue head as active and return it.
            usb_uhci_queue_head_t *queueHead = controller->QueueHeadPool + i;
            memset(queueHead, 0, sizeof(usb_uhci_queue_head_t));
            controller->QueueHeadMap[i] = true;
            return queueHead;
        }
    }

    // No queue head was found.
    panic("UHCI: No more available queue heads!\n");
}

static void usb_uhci_queue_head_free(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead) {
    // Mark queue head as free.
    uint32_t index = ((uintptr_t)queueHead - (uintptr_t)controller->QueueHeadPool) / sizeof(usb_uhci_queue_head_t);
    controller->QueueHeadMap[index] = false;
}

static void usb_uhci_queue_head_add(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead) {
    usb_uhci_queue_head_t *end = (usb_uhci_queue_head_t*)pmm_dma_get_virtual(controller->QueueHead->PreviousQueueHead);  //(usb_uhci_queue_head_t*)pmm_dma_get_virtual(((uint8_t*)controller->QueueHead->PreviousQueueHead) - (uint32_t)(&(((usb_uhci_queue_head_t*)0)->PreviousQueueHead)));

    queueHead->Head = USB_UHCI_FRAME_TERMINATE;
    uint16_t s = inw(USB_UHCI_USBSTS(controller->BaseAddress));
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
    
  //  uint16_t pciS = pci_config_read_word(controller->PciDevice, PCI_REG_STATUS);
    
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
        
    }

if (transferDesc != NULL) {
    kprintf("packet data\n");
    kprintf("0x%X 0x%X 0x%X 0x%X\n", *t1, *t2, *t3, *t4);}

   // kprintf("tick\n");
   // kprintf("next packet: 0x%X\n", transferDesc->LinkPointer);
  //  kprintf("UHCI: packet type: 0x%X\n", transferDesc->PacketType);
    //    kprintf("UHCI: packet pointer: 0x%X\n", transferDesc->BufferPointer);
        kprintf("current frame: %u\n", d);
                kprintf("status: 0x%X\n", s);
       // kprintf("pci status: 0x%X\n", pciS);
       // kprintf("usb cmd: 0x%X\n", inw(USB_UHCI_USBCMD(controller->BaseAddress)));
     //   kprintf("port 1: 0x%X\n", inw(USB_UHCI_PORTSC1(controller->BaseAddress)));

    // Are we done with transfers?
    if (complete) {
        // Remove queue head from schedule.
        kprintf("done!\n");
        kprintf("status: 0x%X\n", s);
      //  kprintf("pci status: 0x%X\n", pciS);
      //  kprintf("usb cmd: 0x%X\n", inw(USB_UHCI_USBCMD(controller->BaseAddress)));
     //   kprintf("port 1: 0x%X\n", inw(USB_UHCI_PORTSC1(controller->BaseAddress)));

        //uint64_t *d = (uint64_t*)pmm_dma_get_virtual(transferDesc->BufferPointer);
        //kprintf_nlock("0x%X\n", *d);

       // while(true);

        // Free transfer descriptors.
        transferDesc = (usb_uhci_transfer_desc_t*)pmm_dma_get_virtual(queueHead->TransferDescHead);
        while (transferDesc != NULL) {
            // Free transcript descriptor.
            usb_uhci_transfer_desc_t* nextDesc = (usb_uhci_transfer_desc_t*)pmm_dma_get_virtual(transferDesc->LinkPointer & ~0xF);
            usb_uhci_transfer_desc_free(controller, transferDesc);
            transferDesc = nextDesc;
        }

        // Free queue head.
        usb_uhci_queue_head_free(controller, queueHead);
       // usb_uhci_queue_head_t *end = (usb_uhci_queue_head_t*)pmm_dma_get_virtual(controller->QueueHead);  //(usb_uhci_queue_head_t*)pmm_dma_get_virtual(((uint8_t*)controller->QueueHead->PreviousQueueHead) - (uint32_t)(&(((usb_uhci_queue_head_t*)0)->PreviousQueueHead)));

        //queueHead->Head = USB_UHCI_FRAME_TERMINATE;
        controller->QueueHead->Head = USB_UHCI_FRAME_TERMINATE;// (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead) | USB_UHCI_FRAME_QUEUE_HEAD;
        controller->QueueHead->PreviousQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)controller->QueueHead);
        controller->QueueHead->NextQueueHead = (uint32_t)pmm_dma_get_phys((uintptr_t)controller->QueueHead);
    }
    return complete;
}

static bool usb_uhci_queue_head_wait(usb_uhci_controller_t *controller, usb_uhci_queue_head_t *queueHead) {
    // Wait for all queue heads to be processed.
    bool status;
    while (!usb_uhci_queue_head_process(controller, queueHead, &status));
    return status;
}

static bool usb_uhci_device_control(usb_device_t *device, uint8_t endpoint, usb_request_t *request, void *data, uint16_t length) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)device->Controller;

    // Get device attributes.
    bool lowSpeed = device->Speed == USB_SPEED_LOW;
    uint8_t address = device->Address;
    uint8_t maxPacketSize = device->MaxPacketSize;

    // Create first transfer descriptor.
    usb_uhci_transfer_desc_t *transferDesc = usb_uhci_transfer_desc_alloc(controller, device, NULL, USB_UHCI_TD_PACKET_SETUP, 0, false, request, sizeof(usb_request_t));
    usb_uhci_transfer_desc_t *headDesc = transferDesc;
    usb_uhci_transfer_desc_t *prevDesc = NULL;

    // Create SETUP packet.
    kprintf("Setting up setup packet for %s speed...\n", lowSpeed ? "low" : "full");
    
    //usb_uhci_transfer_desc_init(transferDesc, NULL, lowSpeed, address, endpoint, false, USB_UHCI_TD_PACKET_SETUP, sizeof(usb_request_t), request);
    prevDesc = transferDesc;

    // Determine whether packets are to be IN or OUT.
    uint8_t packetType = request->Type & USB_REQUEST_TYPE_DEVICE_TO_HOST ? USB_UHCI_TD_PACKET_IN : USB_UHCI_TD_PACKET_OUT;

    // Create packets for data.
    uint8_t *packetPtr = (uint8_t*)data;
    uint8_t *endPtr = packetPtr + length;
    bool toggle = false;
    while (packetPtr < endPtr) {
        // Allocate a transfer descriptor for packet.
        //transferDesc = usb_uhci_transfer_desc_alloc(controller);

        // Toggle toggle bit.
        toggle = !toggle;

        // Determine size of packet, and cut it down if above max size.
        uint8_t packetSize = endPtr - packetPtr;
        if (packetSize > maxPacketSize)
            packetSize = maxPacketSize;
        
        // Create packet.
        kprintf("packet pointer: 0x%p\n", packetPtr);
        //usb_uhci_transfer_desc_init(transferDesc, prevDesc, lowSpeed, address, endpoint, toggle, packetType, packetSize, packetPtr);
        transferDesc = usb_uhci_transfer_desc_alloc(controller, device, prevDesc, packetType, 0, toggle, packetPtr, packetSize);

        // Move to next packet.
        packetPtr += packetSize;
        prevDesc = transferDesc;
    }

   /* if (length == 0) {
        transferDesc = usb_uhci_transfer_desc_alloc(controller, device, prevDesc, packetType, 0, true, NULL, 0);

        // Move to next packet.
        prevDesc = transferDesc;
    }*/

    // Create status packet.
    //transferDesc = usb_uhci_transfer_desc_alloc(controller);
    packetType = request->Type & USB_REQUEST_TYPE_DEVICE_TO_HOST ? USB_UHCI_TD_PACKET_OUT : USB_UHCI_TD_PACKET_IN;
    //usb_uhci_transfer_desc_init(transferDesc, prevDesc, lowSpeed, address, endpoint, true, packetType, 0, NULL);
    transferDesc = usb_uhci_transfer_desc_alloc(controller, device, prevDesc, packetType, 0, true, NULL, 0);

    // Create queue head that starts with first transfer descriptor.
    usb_uhci_queue_head_t *queueHead = usb_uhci_queue_head_alloc(controller);
    queueHead->TransferDescHead = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);
    queueHead->Element = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);

    // Add queue head to schedule and wait for completion.
    kprintf("UHCI: Adding packet....\n");
    usb_uhci_queue_head_add(controller, queueHead);
    return usb_uhci_queue_head_wait(controller, queueHead);
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
   // uint16_t status = inw(portReg);
  //  status &= USB_UHCI_PORTSC_MASK;
    outw(portReg, USB_UHCI_PORTSC_RESET);
    sleep(50);

    // Clear reset bit.
  //  status = ;
  //  status &= USB_UHCI_PORTSC_MASK;
    outw(portReg, inw(portReg) & ~USB_UHCI_PORTSC_RESET);
   // sleep(100);

   while (inw(portReg) & USB_UHCI_PORTSC_RESET);
   sleep(10);

   // Enable port.
   outw(portReg, USB_UHCI_PORTSC_PRESENT_CHANGE | USB_UHCI_PORTSC_ENABLE_CHANGE | USB_UHCI_PORTSC_ENABLED);
   sleep(200);


    return inw(portReg);

    // Reset port.
    //usb_uhci_write_port(portReg, USB_UHCI_PORT_STATUS_RESET, false);
  //  sleep(58);
  //  usb_uhci_write_port(portReg, USB_UHCI_PORT_STATUS_RESET, true);
  //  sleep(1000);

    // Wait for port to be connected and enabled.
   /* for (uint16_t i = 0; i < 100; i++) {
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
    return status;*/
}

static void usb_uhci_reset_global(usb_uhci_controller_t *controller) {
    // Save SOF register.
    uint8_t sof = inb(USB_UHCI_SOFMOD(controller->BaseAddress));

    // Perform global reset.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), inw(USB_UHCI_USBCMD(controller->BaseAddress)) | USB_UHCI_STATUS_GLOBAL_RESET);
    sleep(100);

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
    if (run)
        status |= USB_UHCI_STATUS_RUN;
    else
        status &= ~USB_UHCI_STATUS_RUN;

    // Write to status register.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), status);
}
usb_uhci_controller_t *testcontroller;
void usb_callback() {
    kprintf_nlock("IRQ usb\n");
    PciDevice *device = testcontroller->PciDevice;
    kprintf_nlock("status: 0x%X\n", pci_config_read_word(device, PCI_REG_STATUS));
    kprintf_nlock("usb status: 0x%X\n", inw(USB_UHCI_USBSTS(testcontroller->BaseAddress)));
    outw(USB_UHCI_USBSTS(testcontroller->BaseAddress), 0x3F);
    pci_config_write_word(device, PCI_REG_STATUS, 0x8);
}



void usb_uhci_init(PciDevice *device) {
    kprintf("UHCI: Initializing...\n");

    // Create UHCI controller object.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)kheap_alloc(sizeof(usb_uhci_controller_t));
    memset(controller, 0, sizeof(usb_uhci_controller_t));
    device->DriverObject = controller;
    testcontroller = controller;

    // Store PCI device object, port I/O base address, and specification version.
    controller->PciDevice = device;
    controller->BaseAddress = (device->BAR[4] & PCI_BAR_PORT_MASK);
    controller->SpecVersion = pci_config_read_byte(device, USB_UHCI_PCI_REG_RELEASE_NUM);
    kprintf("UHCI: Controller located at 0x%X, version 0x%X.\n", controller->BaseAddress, controller->SpecVersion);

    // Enable PCI busmaster and port I/O, if not already enabled.
    uint16_t pciCmd = pci_config_read_word(device, PCI_REG_COMMAND);
    pci_config_write_word(device, PCI_REG_COMMAND, pciCmd | 0x01 | 0x04);
    kprintf("UHCI: Original PCI command register value: 0x%X\n", pciCmd);
    kprintf("UHCI: Current PCI command register value: 0x%X\n", pci_config_read_word(device, PCI_REG_COMMAND));

    // Get value of legacy status register.
    uint16_t legacyReg = pci_config_read_word(device, USB_UHCI_PCI_REG_LEGACY);

    // Perform a global reset of the UHCI controller.
    kprintf("UHCI: Performing global reset...\n");
    usb_uhci_reset_global(controller);

    // Reset controller status bits.
    outw(USB_UHCI_USBSTS(controller->BaseAddress), USB_UHCI_STS_MASK);
    sleep(1);

    // Reset legacy status.
    pci_config_write_word(device, USB_UHCI_PCI_REG_LEGACY, USB_UHCI_PCI_LEGACY_STATUS);

    // Reset host controller.
    kprintf("UHCI: Resetting controller...\n");
    if (!usb_uhci_reset(controller)) {
        kprintf("UHCI: Failed to reset controller! Aborting.\n");
        return;
    }

    // Ensure controller and interrupts are disabled.
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0);
    outw(USB_UHCI_USBCMD(controller->BaseAddress), 0);

    // Pull a DMA frame for USB frame storage.
    if (!pmm_dma_get_free_frame(&controller->FrameList))
        panic("UHCI: Couldn't get DMA frame!\n");
    memset(controller->FrameList, 0, PAGE_SIZE_64K);

    // Get pointers to frame list and pools.
    kprintf("UHCI: Frame list located at: 0x%p (0x%X)\n", controller->FrameList, (uint32_t)pmm_dma_get_phys(controller->FrameList));
    controller->TransferDescPool = (usb_uhci_transfer_desc_t*)((uintptr_t)controller->FrameList + (USB_UHCI_FRAME_COUNT * sizeof(uint32_t)));
    controller->QueueHeadPool = (usb_uhci_queue_head_t*)((uintptr_t)controller->TransferDescPool + USB_UHCI_TD_POOL_SIZE);

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

    kprintf("current sof 0x%X\n", inb(USB_UHCI_SOFMOD(controller->BaseAddress)));
    outb(USB_UHCI_SOFMOD(controller->BaseAddress), 0x40);

    // Set frame base address and frame number.
    outl(USB_UHCI_FRBASEADD(controller->BaseAddress), (uint32_t)pmm_dma_get_phys(controller->FrameList));
    outw(USB_UHCI_FRNUM(controller->BaseAddress), 0);

    // Set PIRQ.
    pci_config_write_word(device, USB_UHCI_PCI_REG_LEGACY, USB_UHCI_PCI_LEGACY_PIRQ);

    // Set max packet size to 64 bytes.
    kprintf("UHCI: Starting controller...\n");
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS);
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0xF);
    irqs_install_handler(device->InterruptLine, usb_callback);

    for (uint16_t port = 0; port < 2; port++)
        outw(USB_UHCI_PORTSC1(controller->BaseAddress) + port * 2, USB_UHCI_PORTSC_PRESENT_CHANGE);

    // Force a global resume.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS | USB_UHCI_STATUS_FORCE_GLOBAL_RESUME);
    sleep(20);
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS);
    sleep(100);


    // Start up controller.
  //  usb_uhci_change_state(controller, true);

    // Query root ports.
    for (uint16_t port = 0; port < 2; port++) {
        // Reset the port and get its status.
        uint16_t portStatus = usb_uhci_reset_port(controller, port);
      //  sleep(100);
        //portStatus = usb_uhci_reset_port(controller, port);
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
                usbDevice->Speed = ((portStatus & USB_UHCI_PORTSC_LOW_SPEED) == 0) ? USB_SPEED_FULL : USB_SPEED_LOW;
                usbDevice->MaxPacketSize = 8;
                usbDevice->Address = 0;

            for (int e = 0; e < 10; e++) {
                    usb_request_t *req = (usb_request_t*)((uintptr_t)controller->QueueHeadPool + USB_UHCI_QH_POOL_SIZE);
                memset(req, 0, sizeof(usb_request_t));
                req->Type = USB_REQUEST_TYPE_DEVICE_TO_HOST | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TYPE_REC_DEVICE;
                req->Request = USB_REQUEST_GET_DESCRIPTOR;
                req->Value = 0x1 << 8;
                req->Index = 0;
                req->Length = 8;//sizeof(usb_descriptor_device_t);
                
                usb_descriptor_device_t *desc = (usb_descriptor_device_t*)((uintptr_t)req + PAGE_SIZE_4K);
                memset(desc, 0, sizeof(usb_descriptor_device_t));
              //  sleep(1000);
               // kprintf("speed: 0x%X\n", usbDevice->Speed);
               kprintf("attempting\n");
                if (!usb_uhci_device_control(usbDevice, 0, req, desc, req->Length)) {
                    portStatus = usb_uhci_reset_port(controller, port);
                    continue;
                }
                kprintf("max packet size: %u\n", desc->MaxPacketSize);
                usbDevice->MaxPacketSize = desc->MaxPacketSize;


                memset(req, 0, sizeof(usb_request_t));
                req->Type = USB_REQUEST_TYPE_HOST_TO_DEVICE | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TYPE_REC_DEVICE;
                req->Request = USB_REQUEST_SET_ADDRESS;
                req->Value = port + 1;
                req->Index = 0;
                req->Length = 0;//sizeof(usb_descriptor_device_t);
                
                memset(desc, 0, sizeof(usb_descriptor_device_t));
               // sleep(1000);
                //kprintf("speed: 0x%X\n", usbDevice->Speed);
                usb_uhci_device_control(usbDevice, 0, req, desc, req->Length);
                kprintf("max packet size: %u\n", usbDevice->MaxPacketSize);
                kprintf("address changed");

                usbDevice->Address = 1;
                memset(req, 0, sizeof(usb_request_t));
                req->Type = USB_REQUEST_TYPE_DEVICE_TO_HOST | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TYPE_REC_DEVICE;
                req->Request = USB_REQUEST_GET_DESCRIPTOR;
                req->Value = 0x1 << 8;
                req->Index = 0;
                req->Length = sizeof(usb_descriptor_device_t);
                
                memset(desc, 0, sizeof(usb_descriptor_device_t));
              //  sleep(1000);
                kprintf("speed: 0x%X\n", usbDevice->Speed);
                usb_uhci_device_control(usbDevice, 0, req, desc, req->Length);
                kprintf("max packet size: %u\n", desc->MaxPacketSize);

                kprintf("Vendor: 0x%X\n", desc->VendorId);

                memset(req, 0, sizeof(usb_request_t));
                req->Type = USB_REQUEST_TYPE_DEVICE_TO_HOST | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TYPE_REC_DEVICE;
                req->Request = USB_REQUEST_GET_DESCRIPTOR;
                req->Value = 0x3 << 8 | desc->ProductString;
                req->Index = 0;
                req->Length = 30;
                
                usb_descriptor_string_t *descS = (usb_descriptor_string_t*)((uintptr_t)req + PAGE_SIZE_4K);
                memset(descS, 0, sizeof(usb_descriptor_string_t));
                usb_uhci_device_control(usbDevice, 0, req, descS, req->Length);

                char* dd = (char*)&descS->String;
                kprintf("product string: ");
                for (uint16_t i = 0; i < descS->Length - 2; i++)
                    kprintf("%c", (char)descS->String[i]);
                kprintf("\n");

                while(true);
}   
            }
        }
        else {
            kprintf("UHCI: Port %u is disabled!\n", port + 1);
        }
    }
}