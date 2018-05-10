/*
 * File: ahci.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/storage/ahci/ahci.h>

#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <driver/storage/storage.h>
#include <kernel/memory/paging.h>
#include <driver/pci.h>

void ahci_port_cmd_start(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Wait until port stops.
    while (portMemory->CommandStatus.CommandListRunning);

    // Start port.
    portMemory->CommandStatus.FisReceiveEnabled = true;
    portMemory->CommandStatus.Started = true;
}

void ahci_port_cmd_stop(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Stop port.
    portMemory->CommandStatus.Started = false;

    // Wait until port stops.
    while (portMemory->CommandStatus.FisReceiveRunning
        && portMemory->CommandStatus.CommandListRunning);

    // Stop recieving FISes.
    portMemory->CommandStatus.FisReceiveEnabled = false;
}

void ahci_port_init_memory(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Stop port.
    ahci_port_cmd_stop(ahciPort);

    // Set command list offset (1KB per port).
    portMemory->CommandListBaseAddress = (uint32_t)pmm_dma_get_phys(ahciController->DmaPage + (ahciPort->Number << 10));
    memset((void*)pmm_dma_get_virtual(portMemory->CommandListBaseAddress), 0, AHCI_CMDLIST_SIZE);

    // Set FIS list offset (256 bytes per port).
    portMemory->FisBaseAddress = (uint32_t)pmm_dma_get_phys(ahciController->DmaPage + (32 << 10) + (ahciPort->Number << 8));
    memset((void*)pmm_dma_get_virtual(portMemory->FisBaseAddress), 0, AHCI_FIS_SIZE);

    // Configure first command header. This will need changes if we want to
    // do more than one command at a time.
    ahci_command_header_t *cmdHeaders = (ahci_command_header_t*)pmm_dma_get_virtual(portMemory->CommandListBaseAddress);

    cmdHeaders[0].PhyRegionDescTableLength = 8;
    cmdHeaders[0].CommandTableBaseAddress = (uint32_t)pmm_dma_get_phys(ahciController->DmaPage + (40 << 10) + (ahciPort->Number << 8));
    memset((void*)pmm_dma_get_virtual(cmdHeaders[0].CommandTableBaseAddress), 0, 256);

    // Start port.
    ahci_port_cmd_start(ahciPort);
}

bool ahci_probe_port(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Check status.
    ahciPort->Type = AHCI_DEV_TYPE_NONE;
    if (portMemory->SataStatus.DeviceDetection != AHCI_SATA_STATUS_DETECT_CONNECTED)
        return false;
    if (portMemory->SataStatus.InterfacePowerManagement != AHCI_SATA_STATUS_IPM_ACTIVE)
        return false; 

    // Determine type.
    switch (portMemory->Signature.Value) {
        case AHCI_SIG_ATA:
            ahciPort->Type = AHCI_DEV_TYPE_SATA;
            return true;

        case AHCI_SIG_ATAPI:
            ahciPort->Type = AHCI_DEV_TYPE_SATA_ATAPI;
            return true;
    }

    return false;
}

bool ahci_init(pci_device_t *pciDevice) {
    // Is the PCI device an ATA controller?
    if (!(pciDevice->Class == PCI_CLASS_MASS_STORAGE && pciDevice->Subclass == PCI_SUBCLASS_MASS_STORAGE_SATA && pciDevice->Interface == PCI_INTERFACE_MASS_STORAGE_SATA_VENDOR_AHCI))
        return false;

    // Check address type.
    if (!(!pciDevice->BaseAddresses[5].PortMapped && pciDevice->BaseAddresses[5].BaseAddress != 0))
        return false;

    // Create controller object and map to memory.
    kprintf("AHCI: Initializing controller at 0x%X...\n", pciDevice->BaseAddresses[5].BaseAddress);
    ahci_controller_t *ahciController = (ahci_controller_t*)kheap_alloc(sizeof(ahci_controller_t));
    memset(ahciController, 0, sizeof(ahci_controller_t));
    ahciController->BaseAddress = pciDevice->BaseAddresses[5].BaseAddress;
    ahciController->BasePointer = (uint32_t*)paging_device_alloc(ahciController->BaseAddress, ahciController->BaseAddress);
    ahciController->Memory = (ahci_memory_t*)ahciController->BasePointer;

    // Get DMA page (TODO: 64-bit capable controller doesn't have to use that).
    // Pull a DMA frame for USB frame storage.
    if (!pmm_dma_get_free_frame(&ahciController->DmaPage))
        panic("AHCI: Couldn't get DMA frame!\n");
    memset((void*)ahciController->DmaPage, 0, PAGE_SIZE_64K);

    // Get port count.
    ahciController->PortCount = ahciController->Memory->Capabilities.PortCount + 1;
    ahciController->Ports = (ahci_port_t**)kheap_alloc(sizeof(ahci_port_t*) * ahciController->PortCount);
    memset(ahciController->Ports, 0, sizeof(ahci_port_t*) * ahciController->PortCount);

    // Create command list buffers.
    //ahciController->CommandListBuffers = (uintptr_t*)kheap_alloc(sizeof(uintptr_t) * ahciController->PortCount);


    // Create ports.
    uint32_t enabledPorts = 0;
    for (uint8_t port = 0; port < ahciController->PortCount; port++) {
        if (ahciController->Memory->PortsImplemented & (1 << port)) {
            ahciController->Ports[port] = (ahci_port_t*)kheap_alloc(sizeof(ahci_port_t));
            memset(ahciController->Ports[port], 0, sizeof(ahci_port_t));
            ahciController->Ports[port]->Controller = ahciController;
            ahciController->Ports[port]->Number = port;

            // Probe port.
            if (ahci_probe_port(ahciController->Ports[port])) {
                switch (ahciController->Ports[port]->Type) {
                    case AHCI_DEV_TYPE_SATA:
                        kprintf("AHCI: Found SATA drive on port %u.\n", port);
                        break;

                    case AHCI_DEV_TYPE_SATA_ATAPI:
                        kprintf("AHCI: Found SATA ATAPI drive on port %u.\n", port);
                        break;
                }
            }

            // Stop and initialize port's memory.
            ahci_port_init_memory(ahciController->Ports[port]);
            enabledPorts++;
        }
    }

    // Print info.
    kprintf("AHCI: Version major 0x%X, minor 0x%X\n", ahciController->Memory->Version.Major, ahciController->Memory->Version.Minor);
    kprintf("AHCI: Total ports: %u (%u enabled)\n", ahciController->PortCount, enabledPorts);
}
