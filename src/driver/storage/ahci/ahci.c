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

#include <driver/storage/ata/ata.h>
#include <driver/storage/ata/ata_commands.h>

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
    uint32_t timeout = 500;
    while (portMemory->CommandStatus.FisReceiveRunning && portMemory->CommandStatus.CommandListRunning) {
        // Was the timeout reached?
        if (timeout == 0) {
            kprintf("AHCI: Timeout waiting for port %u to stop!\n", ahciPort->Number);
            break;
        }

        sleep(1);
        timeout--;
    }

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

static bool ahci_port_reset(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Stop port.
    ahci_port_cmd_stop(ahciPort);

    // Enable reset bit for 10ms (as per 10.4.2 Port Reset).
    kprintf("AHCI: Resetting port %u...\n", ahciPort->Number);
    portMemory->SataControl.DeviceDetectionInitialization = AHCI_SATA_STATUS_DETECT_INIT_RESET;
    sleep(10);
    portMemory->SataControl.DeviceDetectionInitialization = AHCI_SATA_STATUS_DETECT_INIT_NO_ACTION;

    // Wait for port to be ready.
    uint32_t timeout = 1000;
    while (portMemory->SataStatus.Data.DeviceDetection != AHCI_SATA_STATUS_DETECT_CONNECTED) {
        // Was the timeout reached?
        if (timeout == 0) {
            kprintf("AHCI: Timeout waiting for port %u to reset!\n", ahciPort->Number);
            return false;
        }

        sleep(1);
        timeout--;
    }

    // Restart port.
    ahci_port_cmd_start(ahciPort);

    // Clear port error register.
    portMemory->SataError.RawValue = -1;

    // Wait for device to be ready.
    timeout = 1000;
    while (portMemory->TaskFileData.Status.Data.Busy || portMemory->TaskFileData.Status.Data.DataRequest
        || portMemory->TaskFileData.Status.Data.Error) {
        // Was the timeout reached?
        if (timeout == 0) {
            kprintf("AHCI: Timeout waiting for driver on port %u to be ready!\n", ahciPort->Number);
            return false;
        }

        sleep(1);
        timeout--;
    }
    return true;
}

static bool ahci_probe_port(ahci_port_t *ahciPort) {
    // Get controller and port.
    ahci_controller_t *ahciController = ahciPort->Controller;
    ahci_port_memory_t *portMemory = ahciController->Memory->Ports + ahciPort->Number;

    // Check status.
    ahciPort->Type = AHCI_DEV_TYPE_NONE;
    if (portMemory->SataStatus.Data.DeviceDetection != AHCI_SATA_STATUS_DETECT_CONNECTED)
        return false;
    if (portMemory->SataStatus.Data.InterfacePowerManagement != AHCI_SATA_STATUS_IPM_ACTIVE)
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
    // ICheck that the device is an AHCI controller, and that the BAR is correct.
    if (!(pciDevice->Class == PCI_CLASS_MASS_STORAGE && pciDevice->Subclass == PCI_SUBCLASS_MASS_STORAGE_SATA && pciDevice->Interface == PCI_INTERFACE_MASS_STORAGE_SATA_VENDOR_AHCI))
        return false;
    if (!(!pciDevice->BaseAddresses[5].PortMapped && pciDevice->BaseAddresses[5].BaseAddress != 0))
        return false;

    // Create controller object and map to memory.
    kprintf("AHCI: Initializing controller at 0x%X...\n", pciDevice->BaseAddresses[5].BaseAddress);
    ahci_controller_t *ahciController = (ahci_controller_t*)kheap_alloc(sizeof(ahci_controller_t));
    memset(ahciController, 0, sizeof(ahci_controller_t));
    ahciController->BaseAddress = pciDevice->BaseAddresses[5].BaseAddress;
    ahciController->Memory = (ahci_memory_t*)((uintptr_t)paging_device_alloc(MASK_PAGE_4K(ahciController->BaseAddress), MASK_PAGE_4K(ahciController->BaseAddress))
        + MASK_PAGEFLAGS_4K(ahciController->BaseAddress));

    kprintf("AHCI: Capabilities: 0x%X.\n", ahciController->Memory->Capabilities.RawValue);
    kprintf("AHCI: Current AHCI controller setting: %s.\n", ahciController->Memory->GlobalControl.AhciEnabled ? "on" : "off");
    if (ahciController->Memory->CapabilitiesExtended.Handoff)
        kprintf("AHCI: BIOS handoff required.\n");

    // Enable AHCI on controller.
    ahciController->Memory->GlobalControl.AhciEnabled = true;

    // Get port count and create port pointer array.
    ahciController->PortCount = ahciController->Memory->Capabilities.Data.PortCount + 1;
    ahciController->Ports = (ahci_port_t**)kheap_alloc(sizeof(ahci_port_t*) * ahciController->PortCount);
    memset(ahciController->Ports, 0, sizeof(ahci_port_t*) * ahciController->PortCount);

    // Page to allocate command lists from.
    uint32_t commandListPage = pmm_pop_frame_nonlong();
    ahci_command_header_t *commandLists = (ahci_command_header_t*)paging_device_alloc(commandListPage, commandListPage);
    memset(commandLists, 0, PAGE_SIZE_4K);
    uint8_t commandListsAllocated = 0;
    const uint8_t maxCommandLists = PAGE_SIZE_4K / AHCI_COMMAND_LIST_SIZE; // 4 is the max that can be allocated from a 4KB page.

    // Page to allocated the recieved FIS structures from.
    uint32_t receivedFisPage = pmm_pop_frame_nonlong();
    ahci_received_fis_t *recievedFises = (ahci_received_fis_t*)paging_device_alloc(receivedFisPage, receivedFisPage);
    memset(recievedFises, 0, PAGE_SIZE_4K);
    uint8_t recievedFisesAllocated = 0;
    const uint8_t maxRecievedFises = PAGE_SIZE_4K / sizeof(ahci_received_fis_t); // 16 is the max that can be allocated from a 4KB page.

    // Detect and create ports.
    uint32_t enabledPorts = 0;
    for (uint8_t port = 0; port < ahciController->PortCount; port++) {
        if (ahciController->Memory->PortsImplemented & (1 << port)) {
            // If no more command lists can be allocated, pop another page.
            if (commandListsAllocated == maxCommandLists) {
                commandListPage = pmm_pop_frame_nonlong();
                commandLists = (ahci_command_header_t*)paging_device_alloc(commandListPage, commandListPage);
                memset(commandLists, 0, PAGE_SIZE_4K);
                commandListsAllocated = 0;
            }

            // If no more recived FIS structures can be allocated, pop another page.
            if (recievedFisesAllocated == maxRecievedFises) {
                receivedFisPage = pmm_pop_frame_nonlong();
                recievedFises = (ahci_received_fis_t*)paging_device_alloc(receivedFisPage, receivedFisPage);
                memset(recievedFises, 0, PAGE_SIZE_4K);
                recievedFisesAllocated = 0;
            }

            // Create port object.
            ahciController->Ports[port] = (ahci_port_t*)kheap_alloc(sizeof(ahci_port_t));
            memset(ahciController->Ports[port], 0, sizeof(ahci_port_t));
            ahciController->Ports[port]->Controller = ahciController;
            ahciController->Ports[port]->Number = port;
            ahciController->Ports[port]->CommandList = commandLists + (AHCI_COMMAND_LIST_COUNT * commandListsAllocated);
            commandListsAllocated++;
            ahciController->Ports[port]->ReceivedFis = recievedFises + recievedFisesAllocated;
            recievedFisesAllocated++;

            // Initialize port's memory.
            ahci_port_init_memory(ahciController->Ports[port]);

            // Reset port.
            ahci_port_reset(ahciController->Ports[port]);

            // Probe port.
            kprintf("AHCI: Probing port (status 0x%X) %u...\n", ahciController->Memory->Ports[port].SataStatus.Data.DeviceDetection, port);
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
            enabledPorts++;
        }
    }

    kprintf("AHCI: Version major 0x%X, minor 0x%X\n", ahciController->Memory->Version.Major, ahciController->Memory->Version.Minor);
    kprintf("AHCI: Total ports: %u (%u enabled)\n", ahciController->PortCount, enabledPorts);

    ahci_port_t *hddPort = ahciController->Ports[0];
    

    kprintf("moving on\n");
    uint32_t cmdTablePage = pmm_pop_frame_nonlong();
    ahci_command_table_t *cmdTable = (ahci_command_table_t*)paging_device_alloc(cmdTablePage, cmdTablePage);
    memset(cmdTable, 0, PAGE_SIZE_4K);

    uint32_t cmdTable2Page = pmm_pop_frame_nonlong();
    ahci_command_table_t *cmdTable2 = (ahci_command_table_t*)paging_device_alloc(cmdTable2Page, cmdTable2Page);
    memset(cmdTable2, 0, PAGE_SIZE_4K);
    uint32_t ss = sizeof(ahci_received_fis_t);
    
    uint32_t dataPage = pmm_pop_frame_nonlong();
    uint16_t *dataPtr = (uint16_t*)paging_device_alloc(dataPage, dataPage);
    memset(dataPtr, 0, 0x1000);

    uint32_t data2Page = pmm_pop_frame_nonlong();
    uint16_t *data2Ptr = (uint16_t*)paging_device_alloc(data2Page, data2Page);
    memset(data2Ptr, 0, 0x1000);

    ata_identify_result_2_t* ata = (ata_identify_result_2_t*)dataPtr;
    uint32_t fff = sizeof(ata_identify_result_2_t);

    hddPort->CommandList[0].CommandTableBaseAddress = cmdTablePage;
    hddPort->CommandList[0].CommandFisLength = 4;
    hddPort->CommandList[0].PhyRegionDescTableLength = 1;
    hddPort->CommandList[0].Reset = true;
    hddPort->CommandList[0].ClearBusyUponOk = true;
    cmdTable->PhysRegionDescTable[0].DataBaseAddress = dataPage;
    cmdTable->PhysRegionDescTable[0].DataByteCount = 0x1000 - 1;
    ahci_fis_reg_host_to_device_t *h2d = (ahci_fis_reg_host_to_device_t*)&cmdTable->CommandFis;
    h2d->FisType = 0x27;
    h2d->IsCommand = false;
    h2d->ControlReg = 0x02;

    hddPort->CommandList[1].CommandTableBaseAddress = cmdTable2Page;
    hddPort->CommandList[1].CommandFisLength = 4;
    hddPort->CommandList[1].PhyRegionDescTableLength = 1;
    hddPort->CommandList[1].Reset = false;
    hddPort->CommandList[1].ClearBusyUponOk = false;
    cmdTable2->PhysRegionDescTable[0].DataBaseAddress = data2Page;
    cmdTable2->PhysRegionDescTable[0].DataByteCount = 0x1000 - 1;
    ahci_fis_reg_host_to_device_t *h2d2 = (ahci_fis_reg_host_to_device_t*)&cmdTable->CommandFis;
    h2d2->FisType = 0x27;
    h2d2->IsCommand = false;
    h2d2->ControlReg = 0x00;

    // enable commands.
    /*ahciController->Memory->Ports[0].CommandsIssued = 3;
    while (true) {
        if ((ahciController->Memory->Ports[0].CommandsIssued & 3) == 0)
            break;

        if ((ahciController->Memory->Ports[0].TaskFileData.Error))
        {
            kprintf("Error: 0x%X\n", ahciController->Memory->Ports[0].TaskFileData.Error);
            break;
        }

        sleep(1000);
        kprintf("AHCI: status 0x%X, error 0x%X\n", ahciController->Memory->Ports[0].TaskFileData.Status, ahciController->Memory->Ports[0].TaskFileData.Error);
    }*/
    memset(cmdTable, 0, PAGE_SIZE_4K);

    // Print info.
    kprintf("AHCI port ssts 0x%X\n", ahciController->Memory->Ports[0].SataStatus.RawValue);
    kprintf("AHCI: status 0x%X, error 0x%X\n", ahciController->Memory->Ports[0].TaskFileData.Status.RawValue, ahciController->Memory->Ports[0].TaskFileData.Error);
    kprintf("AHCI: Version major 0x%X, minor 0x%X\n", ahciController->Memory->Version.Major, ahciController->Memory->Version.Minor);
    kprintf("AHCI: Total ports: %u (%u enabled)\n", ahciController->PortCount, enabledPorts);

    hddPort->CommandList[0].CommandTableBaseAddress = cmdTablePage;
    hddPort->CommandList[0].CommandFisLength = 4;
    hddPort->CommandList[0].PhyRegionDescTableLength = 1;
    hddPort->CommandList[0].Reset = false;
    hddPort->CommandList[0].ClearBusyUponOk = false;
    cmdTable->PhysRegionDescTable[0].DataBaseAddress = dataPage;
    cmdTable->PhysRegionDescTable[0].DataByteCount = 0x1000 - 1;
    h2d->FisType = 0x27;
    h2d->ControlReg = 0x00;
    h2d->IsCommand = true;
    h2d->CommandReg = 0xEC;

    // enable command.
    ahciController->Memory->Ports[0].InterruptsStatus.RawValue = -1;
    ahciController->Memory->Ports[0].SataError.RawValue = -1;
    kprintf("int 0x%X\n", ahciController->Memory->Ports[0].InterruptsEnabled.RawValue);
    ahciController->Memory->Ports[0].CommandsIssued = 1;
    while (true) {
        if ((ahciController->Memory->Ports[0].CommandsIssued & 1) == 0)
            break;

        if ((ahciController->Memory->Ports[0].TaskFileData.Error))
        {
           // kprintf("Error: 0x%X\n", ahciController->Memory->Ports[0].TaskFileData.Error);
          //  break;
        }

        sleep(1000);
        kprintf("AHCI: status 0x%X, error 0x%X\n", ahciController->Memory->Ports[0].TaskFileData.Status.RawValue, ahciController->Memory->Ports[0].TaskFileData.Error);
        kprintf("AHCI: general: 0x%X\n", ata->GeneralConfig);
        kprintf("AHCI: int status 0x%X\n", ahciController->Memory->Ports[0].InterruptsStatus.RawValue);
        kprintf("AHCI sata error 0x%X\n", ahciController->Memory->Ports[0].SataError.RawValue);
       // kprintf("in pio: %s\n", )
    }

    char model[ATA_MODEL_LENGTH+1];
    strncpy(model, ata->Model, ATA_MODEL_LENGTH);
    model[ATA_MODEL_LENGTH] = '\0';


    kprintf("AHCI: Model: %s\n", model);
    kprintf("AHCI: Integrity: 0x%X\n", ata->ChecksumValidityIndicator);
    while(true);


}
