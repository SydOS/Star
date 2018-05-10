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

static inline uint32_t ahci_read(ahci_controller_t *ahciController, uint16_t reg) {
    // Read from controller's memory mapped region.
    return *(volatile uint32_t*)((uintptr_t)ahciController->BasePointer + reg);
}

static inline uint32_t ahci_read_port(ahci_port_t *ahciPort, uint16_t reg) {
    return ahci_read(ahciPort->Controller, AHCI_REG_PORT_OFFSET + (ahciPort->Number * AHCI_REG_PORT_MULT) + reg);
}

ahci_host_cap_t ahci_get_cap(ahci_controller_t *ahciController) {
    uint32_t value = ahci_read(ahciController, AHCI_REG_HOST_CAP);
    return *(volatile ahci_host_cap_t*)&value;
}

uint32_t ahci_version(ahci_controller_t *ahciController) {
    return ahci_read(ahciController, AHCI_REG_VERSION);
}

bool ahci_probe_port(ahci_port_t *ahciPort) {
    // Get controller.
    ahci_controller_t *ahciController = ahciPort->Controller;

    // Check status.
    ahciPort->Type = AHCI_DEV_TYPE_NONE;
    if (ahciController->Memory->Ports[ahciPort->Number].SataStatus.DeviceDetection != AHCI_SATA_STATUS_DETECT_CONNECTED)
        return false;
    if (ahciController->Memory->Ports[ahciPort->Number].SataStatus.InterfacePowerManagement != AHCI_SATA_STATUS_IPM_ACTIVE)
        return false; 

    // Determine type.
    switch (ahciController->Memory->Ports[ahciPort->Number].Signature.Value) {
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
    ahci_host_cap_t caps = ahci_get_cap(ahciController);
    ahciController->PortCount = caps.PortCount + 1;
    ahciController->Ports = (ahci_port_t**)kheap_alloc(sizeof(ahci_port_t*) * ahciController->PortCount);
    memset(ahciController->Ports, 0, sizeof(ahci_port_t*) * ahciController->PortCount);

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
            enabledPorts++;
        }
    }

    // Print info.
    kprintf("AHCI: Version major 0x%X, minor 0x%X\n", ahciController->Memory->Version.Major, ahciController->Memory->Version.Minor);
    kprintf("AHCI: Total ports: %u (%u enabled)\n", ahciController->PortCount, enabledPorts);
}
