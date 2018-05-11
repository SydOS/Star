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

static void ahci_port_cmd_start(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Wait until port stops.
    while (portMemory->CommandStatus.CommandListRunning);

    // Start port.
    portMemory->CommandStatus.FisReceiveEnabled = true;
    portMemory->CommandStatus.Started = true;
}

static void ahci_port_cmd_stop(ahci_port_t *ahciPort) {
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

static void ahci_port_init_memory(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Stop port.
    ahci_port_cmd_stop(ahciPort);

    // Get physical addresses of command list.
    uint64_t commandListPhys = 0;
    if (!paging_get_phys((uintptr_t)ahciPort->CommandList, &commandListPhys))
        panic("AHCI: Attempted to use nonpaged address for command list!\n");
    portMemory->CommandListBaseAddress = commandListPhys;

    // Get physical addresses of received FISes.
    uint64_t fisPhys = 0;
    if (!paging_get_phys((uintptr_t)ahciPort->ReceivedFis, &fisPhys))
        panic("AHCI: Attempted to use nonpaged address for command list!\n");
    portMemory->FisBaseAddress = fisPhys;

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

    // Get port count.
    ahciController->PortCount = ahciController->Memory->Capabilities.PortCount + 1;
    ahciController->Ports = (ahci_port_t**)kheap_alloc(sizeof(ahci_port_t*) * ahciController->PortCount);
    memset(ahciController->Ports, 0, sizeof(ahci_port_t*) * ahciController->PortCount);

    uint32_t commandListPage = pmm_pop_frame_nonlong();
    ahci_command_header_t *commandList = (ahci_command_header_t*)paging_device_alloc(commandListPage, commandListPage);
    memset(commandList, 0, PAGE_SIZE_4K);
    uint8_t commandListPageCount = 0;

    uint32_t pageFisList = pmm_pop_frame_nonlong();
    ahci_received_fis_t *fisList = (ahci_received_fis_t*)paging_device_alloc(pageFisList, pageFisList);
    memset(fisList, 0, PAGE_SIZE_4K);
    uint8_t fisListPageCount = 0;

    // Create ports.
    uint32_t enabledPorts = 0;
    for (uint8_t port = 0; port < ahciController->PortCount; port++) {
        if (ahciController->Memory->PortsImplemented & (1 << port)) {
            // If no more command list addresses, pop another page.
            if (commandListPageCount == 4) {
                commandListPage = pmm_pop_frame_nonlong();
                commandList = (ahci_command_header_t*)paging_device_alloc(commandListPage, commandListPage);
                memset(commandList, 0, PAGE_SIZE_4K);
                commandListPageCount = 0;
            }
            if (fisListPageCount == (PAGE_SIZE_4K / sizeof(ahci_received_fis_t))) {
                pageFisList = pmm_pop_frame_nonlong();
                fisList = (ahci_received_fis_t*)paging_device_alloc(pageFisList, pageFisList);
                memset(fisList, 0, PAGE_SIZE_4K);
                fisListPageCount = 0;
            }

            // Create port object.
            ahciController->Ports[port] = (ahci_port_t*)kheap_alloc(sizeof(ahci_port_t));
            memset(ahciController->Ports[port], 0, sizeof(ahci_port_t));
            ahciController->Ports[port]->Controller = ahciController;
            ahciController->Ports[port]->Number = port;
            ahciController->Ports[port]->CommandList = commandList + (AHCI_COMMAND_LIST_COUNT * commandListPageCount);
            commandListPageCount++;
            ahciController->Ports[port]->ReceivedFis = fisList + fisListPageCount;
            fisListPageCount++;

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

            // Initialize port's memory.
            ahci_port_init_memory(ahciController->Ports[port]);
            enabledPorts++;
        }
    }

    ahci_port_t *hddPort = ahciController->Ports[0];

    uint32_t cmdTablePage = pmm_pop_frame_nonlong();
    ahci_command_table_t *cmdTable = (ahci_command_table_t*)paging_device_alloc(cmdTablePage, cmdTablePage);
    memset(cmdTable, 0, PAGE_SIZE_4K);
    uint32_t ss = sizeof(ahci_received_fis_t);
    
    hddPort->CommandList[0].CommandTableBaseAddress = cmdTablePage;
    hddPort->CommandList[0].PhyRegionDescTableLength = 8;
    ahci_fis_reg_host_to_device_t *h2d = (ahci_fis_reg_host_to_device_t*)&cmdTable->CommandFis;
    h2d->FisType = 0x27;
    h2d->IsCommand = true;
    h2d->CommandReg = 0xEC;

    // enable command.
    ahciController->Memory->Ports[0].CommandsIssued = 1;
    while (true) {
        if (ahciController->Memory->Ports[0].CommandsIssued & 1 == 0)
            break;
    }

    // Print info.
    kprintf("AHCI: Version major 0x%X, minor 0x%X\n", ahciController->Memory->Version.Major, ahciController->Memory->Version.Minor);
    kprintf("AHCI: Total ports: %u (%u enabled)\n", ahciController->PortCount, enabledPorts);
}
