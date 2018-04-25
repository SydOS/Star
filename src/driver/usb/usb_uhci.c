#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <string.h>
#include <math.h>
#include <driver/usb/usb_uhci.h>

#include <driver/usb/devices/usb_descriptors.h>
#include <driver/usb/devices/usb_requests.h>
#include <driver/usb/devices/usb_device.h>

#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

static void *usb_uhci_alloc(usb_uhci_controller_t *controller, uint16_t size) {
    // Get total required blocks.
    uint16_t requiredBlocks = DIVIDE_ROUND_UP(size, 8);
    uint16_t currentBlocks = 0;
    uint16_t startIndex = 0;

    // Search for free block range.
    for (uint16_t i = 0; i < USB_UHCI_MEM_BLOCK_COUNT; i++) {
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
        panic("UHCI: No more blocks!\n");

    // Mark blocks as used.
    for (uint16_t i = startIndex; i < startIndex + requiredBlocks; i++)
        controller->MemMap[i] = true;

    // Return pointer to blocks.
    return (void*)((uintptr_t)controller->HeapPool + (startIndex * 8));
}

static void usb_uhci_free(usb_uhci_controller_t *controller, void *pointer, uint16_t size) {
    // Ensure pointer is valid.
    if ((uintptr_t)pointer < (uintptr_t)controller->HeapPool || (uintptr_t)pointer > (uintptr_t)controller->HeapPool + USB_UHCI_MEM_POOL_SIZE || (uintptr_t)pointer & 0x7)
        panic("UHCI: Invalid pointer 0x%p free attempt!", pointer);

    // Get total required blocks.
    uint16_t blocks = DIVIDE_ROUND_UP(size, 8);
    uint16_t startIndex = ((uintptr_t)pointer - (uintptr_t)controller->HeapPool) / 8;

    // Mark blocks as free.
    for (uint16_t i = startIndex; i < startIndex + blocks; i++)
        controller->MemMap[i] = false;
}

static void usb_uhci_address_free(usb_device_t *usbDevice) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)usbDevice->Controller;

    // Mark address free.
    controller->AddressPool[usbDevice->Address - 1] = false;
}

static bool usb_uhci_address_alloc(usb_device_t *usbDevice) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)usbDevice->Controller;

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
                usb_uhci_address_free(usbDevice);

            // Mark address in-use and save to device object.
            controller->AddressPool[i] = true;
            usbDevice->Address = i + 1;
            return true;
        }
    }

    // No address found, too many USB devices attached to host most likely.
    return false;
}

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
   // uint16_t d = inw(USB_UHCI_FRNUM(controller->BaseAddress));
   // uint16_t s = inw(USB_UHCI_USBSTS(controller->BaseAddress));
    
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
            kprintf("packet data\n");
            kprintf("0x%X 0x%X 0x%X 0x%X\n", *t1, *t2, *t3, *t4);
            kprintf("UHCI: packet type: 0x%X\n", transferDesc->PacketType);
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

//if (transferDesc != NULL) {
  //  }

   // kprintf("tick\n");
   // kprintf("next packet: 0x%X\n", transferDesc->LinkPointer);
  //  
    //    kprintf("UHCI: packet pointer: 0x%X\n", transferDesc->BufferPointer);
      //  kprintf("current frame: %u\n", d);
       //         kprintf("status: 0x%X\n", s);
       // kprintf("pci status: 0x%X\n", pciS);
       // kprintf("usb cmd: 0x%X\n", inw(USB_UHCI_USBCMD(controller->BaseAddress)));
     //   kprintf("port 1: 0x%X\n", inw(USB_UHCI_PORTSC1(controller->BaseAddress)));

    // Are we done with transfers?
    if (complete) {
        // Remove queue head from schedule.
     //   kprintf("done!\n");
     //   kprintf("status: 0x%X\n", s);
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

//static bool usb_uhci_device_control(usb_device_t *device, uint8_t endpoint, bool inbound, uint8_t type,
 //   uint8_t recipient, uint8_t requestType, uint8_t valueLo, uint8_t valueHi, uint16_t index, void *buffer, uint16_t length) {

static bool usb_uhci_device_control(usb_device_t* device, usb_endpoint_t *endpoint, usb_control_transfer_t transfer, void *buffer, uint16_t length) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)device->Controller;

    // Temporary buffer.
    void *usbBuffer = NULL;

    // Create USB request.
    usb_request_t *request = (usb_request_t*)usb_uhci_alloc(controller, sizeof(usb_request_t));
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
    usb_uhci_transfer_desc_t *transferDesc = usb_uhci_transfer_desc_alloc(controller, device, NULL, USB_UHCI_TD_PACKET_SETUP, endpoint->Number, false, request, sizeof(usb_request_t));
    usb_uhci_transfer_desc_t *headDesc = transferDesc;
    usb_uhci_transfer_desc_t *prevDesc = transferDesc;

    // Determine whether packets are to be IN or OUT.
    uint8_t packetType = transfer.Inbound ? USB_UHCI_TD_PACKET_IN : USB_UHCI_TD_PACKET_OUT;

    // Create packets for data.
    if (length > 0) {
        // Create temporary buffer that the UHCI controller can use.
        usbBuffer = usb_uhci_alloc(controller, length);
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
            transferDesc = usb_uhci_transfer_desc_alloc(controller, device, prevDesc, packetType, endpoint->Number, toggle, packetPtr, packetSize);

            // Move to next packet.
            packetPtr += packetSize;
            prevDesc = transferDesc;
        }
    }

    // Create status packet.
    packetType = transfer.Inbound ? USB_UHCI_TD_PACKET_OUT : USB_UHCI_TD_PACKET_IN;
    transferDesc = usb_uhci_transfer_desc_alloc(controller, device, prevDesc, packetType, endpoint->Number, true, NULL, 0);

    // Create queue head that starts with first transfer descriptor.
    usb_uhci_queue_head_t *queueHead = usb_uhci_queue_head_alloc(controller);
    queueHead->TransferDescHead = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);
    queueHead->Element = (uint32_t)pmm_dma_get_phys((uintptr_t)headDesc);

    // Add queue head to schedule and wait for completion.
   // kprintf("UHCI: Adding packet....\n");
    usb_uhci_queue_head_add(controller, queueHead);
    bool result = usb_uhci_queue_head_wait(controller, queueHead);

    // Copy data if the operation succeeded.
    if (transfer.Inbound && usbBuffer != NULL && result)
        memcpy(buffer, usbBuffer, length);

    // Free buffers.
    usb_uhci_free(controller, request, sizeof(usb_request_t));
    if (usbBuffer != NULL)
        usb_uhci_free(controller, usbBuffer, length);

    return result;
}

void usb_uhci_device_interrupt_in_start(usb_device_t *device, usb_endpoint_t *endpoint, uint16_t length) {
    // Get the UHCI controller.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)device->Controller;

    // Temporary buffer.
    void *usbBuffer = usb_uhci_alloc(controller, length);

    // Create transfer descriptor.
    usb_uhci_transfer_desc_t *transferDesc = usb_uhci_transfer_desc_alloc(controller, device, NULL, USB_UHCI_TD_PACKET_IN, endpoint->Number, false, usbBuffer, length);

    // Create queue head that starts with transfer descriptor.
    usb_uhci_queue_head_t *queueHead = usb_uhci_queue_head_alloc(controller);
    queueHead->Head = USB_UHCI_FRAME_TERMINATE;
    queueHead->TransferDescHead = (uint32_t)pmm_dma_get_phys((uintptr_t)transferDesc);
    queueHead->Element = (uint32_t)pmm_dma_get_phys((uintptr_t)transferDesc);

    // Store info in endpoint.
    usb_uhci_transfer_info_t *transferInfo = (usb_uhci_transfer_info_t*)kheap_alloc(sizeof(usb_uhci_transfer_info_t));
    transferInfo->QueueHead = queueHead;
    transferInfo->TransferDesc = transferDesc;
    endpoint->TransferInfo = (void*)transferInfo;

    // Add interrupt to schedule using the interval specified. Each frame/interval = 1ms.
    for (uint16_t i = 0; i < USB_UHCI_FRAME_COUNT; i += endpoint->Interval) {
        // If the queue head is the main one, replace with a new one.
        uintptr_t qhAddress = pmm_dma_get_virtual(controller->FrameList[i] & ~0xF);
        if (qhAddress == (uintptr_t)controller->QueueHead) {
            controller->FrameList[i] = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead) | USB_UHCI_FRAME_QUEUE_HEAD;
        }
        else {
            // Add queue head to entry, with existing queue head linked after.
            queueHead->Head = (controller->FrameList[i] & ~0xF) | USB_UHCI_FRAME_QUEUE_HEAD;
            controller->FrameList[i] = (uint32_t)pmm_dma_get_phys((uintptr_t)queueHead) | USB_UHCI_FRAME_QUEUE_HEAD;
        }
    }

  //  bool result = usb_uhci_queue_head_wait(controller, queueHead);

    // Add queue head to schedule and wait for completion.
   // usb_uhci_queue_head_add(controller, queueHead);

    //return result;
}

bool usb_uhci_device_interrupt_in_poll(usb_device_t *device, usb_endpoint_t *endpoint, void *outBuffer, uint16_t length) {
    // Get transfer descriptor.
    usb_uhci_transfer_info_t *transferInfo = (usb_uhci_transfer_info_t*)endpoint->TransferInfo;

    // Is it complete?
    if (transferInfo->TransferDesc->Active)
        return false;

    // Copy data.
    void *usbBuffer = (void*)pmm_dma_get_virtual(transferInfo->TransferDesc->BufferPointer);
    memcpy(outBuffer, usbBuffer, length);

    // Flip toggle bit and reset TD.
    transferInfo->TransferDesc->DataToggle = !transferInfo->TransferDesc->DataToggle;
    transferInfo->TransferDesc->ActualLength = 0;

    // Reset element
    transferInfo->QueueHead->Element = (uint32_t)pmm_dma_get_phys((uintptr_t)transferInfo->TransferDesc);
    transferInfo->TransferDesc->Active = true;
    return true;
}

static uint16_t usb_uhci_reset_port(usb_uhci_controller_t *controller, uint8_t port) {
    // Determine port register.
    uint16_t portReg = USB_UHCI_PORTSC1(controller->BaseAddress) + ((port - 1) * sizeof(uint16_t));

    // Reset port and wait.
    outw(portReg, USB_UHCI_PORTSC_RESET);
    sleep(50);

    // Clear reset bit and wait for port to reset.
    outw(portReg, inw(portReg) & ~USB_UHCI_PORTSC_RESET);
    while (inw(portReg) & USB_UHCI_PORTSC_RESET);
    sleep(10);

    // Enable port.
    outw(portReg, USB_UHCI_PORTSC_PRESENT_CHANGE | USB_UHCI_PORTSC_ENABLE_CHANGE | USB_UHCI_PORTSC_ENABLED);
    sleep(200);

    // Return port status.
    return inw(portReg);
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

usb_uhci_controller_t *testcontroller;
void usb_callback() {
    kprintf_nlock("IRQ usb\n");
    pci_device_t *device = testcontroller->PciDevice;
    kprintf_nlock("status: 0x%X\n", pci_config_read_word(device, PCI_REG_STATUS));
    kprintf_nlock("usb status: 0x%X\n", inw(USB_UHCI_USBSTS(testcontroller->BaseAddress)));
    outw(USB_UHCI_USBSTS(testcontroller->BaseAddress), 0x3F);
    pci_config_write_word(device, PCI_REG_STATUS, 0x8);
}

void usb_uhci_init(pci_device_t *pciDevice) {
    kprintf("UHCI: Initializing...\n");

    // Create UHCI controller object.
    usb_uhci_controller_t *controller = (usb_uhci_controller_t*)kheap_alloc(sizeof(usb_uhci_controller_t));
    memset(controller, 0, sizeof(usb_uhci_controller_t));
    pciDevice->DriverObject = controller;
    testcontroller = controller;

    // Store PCI device object, port I/O base address, and specification version.
    controller->PciDevice = pciDevice;
    controller->BaseAddress = pciDevice->BaseAddresses[4].BaseAddress;
    controller->SpecVersion = pci_config_read_byte(pciDevice, USB_UHCI_PCI_REG_RELEASE_NUM);
    kprintf("UHCI: Controller located at 0x%X, version 0x%X.\n", controller->BaseAddress, controller->SpecVersion);

    // Enable PCI busmaster and port I/O, if not already enabled.
    uint16_t pciCmd = pci_config_read_word(pciDevice, PCI_REG_COMMAND);
    pci_config_write_word(pciDevice, PCI_REG_COMMAND, pciCmd | 0x01 | 0x04);
    kprintf("UHCI: Original PCI command register value: 0x%X\n", pciCmd);
    kprintf("UHCI: Current PCI command register value: 0x%X\n", pci_config_read_word(pciDevice, PCI_REG_COMMAND));

    // Get value of legacy status register.
    uint16_t legacyReg = pci_config_read_word(pciDevice, USB_UHCI_PCI_REG_LEGACY);

    // Perform a global reset of the UHCI controller.
    kprintf("UHCI: Performing global reset...\n");
    usb_uhci_reset_global(controller);

    // Reset controller status bits.
    outw(USB_UHCI_USBSTS(controller->BaseAddress), USB_UHCI_STS_MASK);
    sleep(1);

    // Reset legacy status.
    pci_config_write_word(pciDevice, USB_UHCI_PCI_REG_LEGACY, USB_UHCI_PCI_LEGACY_STATUS);

    // Reset host controller.
    kprintf("UHCI: Resetting controller...\n");
    if (!usb_uhci_reset(controller)) {
        kprintf("UHCI: Failed to reset controller! Aborting.\n");
        kheap_free(controller);
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
    controller->TransferDescPool = (usb_uhci_transfer_desc_t*)((uintptr_t)controller->FrameList + USB_UHCI_FRAME_POOL_SIZE);
    controller->QueueHeadPool = (usb_uhci_queue_head_t*)((uintptr_t)controller->TransferDescPool + USB_UHCI_TD_POOL_SIZE);
    controller->HeapPool = (uint8_t*)((uintptr_t)controller->QueueHeadPool + USB_UHCI_QH_POOL_SIZE);

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

    // Set frame base address and frame number.
    outb(USB_UHCI_SOFMOD(controller->BaseAddress), 0x40);
    outl(USB_UHCI_FRBASEADD(controller->BaseAddress), (uint32_t)pmm_dma_get_phys(controller->FrameList));
    outw(USB_UHCI_FRNUM(controller->BaseAddress), 0);

    // Set PIRQ.
    pci_config_write_word(pciDevice, USB_UHCI_PCI_REG_LEGACY, USB_UHCI_PCI_LEGACY_PIRQ);

    // Start up controller and enable interrupts.
    kprintf("UHCI: Starting controller...\n");
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS);
    outw(USB_UHCI_USBINTR(controller->BaseAddress), 0xF);
    irqs_install_handler(pciDevice->InterruptLine, usb_callback);

    // Clear port status bits.
    for (uint16_t port = 0; port < 2; port++)
        outw(USB_UHCI_PORTSC1(controller->BaseAddress) + port * 2, USB_UHCI_PORTSC_PRESENT_CHANGE);

    // Force a global resume.
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS | USB_UHCI_STATUS_FORCE_GLOBAL_RESUME);
    sleep(20);
    outw(USB_UHCI_USBCMD(controller->BaseAddress), USB_UHCI_STATUS_RUN | USB_UHCI_STATUS_CONFIGURE | USB_UHCI_STATUS_64_BYTE_PACKETS);
    sleep(100);
    kprintf("UHCI: Controller started!\n");

    // Create root USB device.
    controller->RootDevice = (usb_device_t*)kheap_alloc(sizeof(usb_device_t));
    memset(controller->RootDevice, 0, sizeof(usb_device_t));
    if (controller->RootDevice == NULL) {
        kprintf("UHCI: Failed to create root device!\n");
        kheap_free(controller);
        return;
    }

    // No parent means it is on the root hub.
    controller->RootDevice->Parent = NULL;
    controller->RootDevice->Controller = controller;
    controller->RootDevice->AllocAddress = usb_uhci_address_alloc;
    controller->RootDevice->FreeAddress = usb_uhci_address_free;
    controller->RootDevice->ControlTransfer = usb_uhci_device_control;
    controller->RootDevice->InterruptTransferStart = usb_uhci_device_interrupt_in_start;

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
    controller->RootDevice->ProductString = "UHCI Root Hub";
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

    // Query root ports.
    for (uint16_t port = 1; port <= 2; port++) {
        // Reset the port and get its status.
        uint16_t portStatus = usb_uhci_reset_port(controller, port);
        sleep(20);
        kprintf("UHCI: Port status for port %u: 0x%X\n", port, portStatus);

        // Is the port enabled?
        if (portStatus & USB_UHCI_PORTSC_ENABLED) {
            kprintf("UHCI: Port %u is enabled, at %s speed!\n", port, (portStatus & USB_UHCI_PORTSC_LOW_SPEED) ? "low" : "full");
            usb_device_t *usbDevice = usb_device_create(controller->RootDevice, port, (portStatus & USB_UHCI_PORTSC_LOW_SPEED) ? USB_SPEED_LOW : USB_SPEED_FULL);
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
        else {
            kprintf("UHCI: Port %u is disabled!\n", port);
        }
    }
}