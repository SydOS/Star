/*
 * File: pci.c
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
#include <io.h>
#include <kprint.h>
#include <driver/pci.h>

#include <kernel/memory/kheap.h>
#include <kernel/interrupts/ioapic.h>
#include <driver/vga.h>

#include <kernel/interrupts/irqs.h>

#include <acpi.h>

// PCI devices.
pci_device_t *PciDevices = NULL;

static bool pci_irq_callback(irq_regs_t *regs, uint8_t irqNum) {
    kprintf("PCI: IRQ %u raised!\n", irqNum);

    // Call handlers of devices that are on the raised IRQ, until the IRQ is handled.
    pci_device_t *pciDevice = PciDevices;
    while (pciDevice != NULL) {
        // Ensure device's IRQ matches and there is an interrupt handler.
        if (pciDevice->InterruptNo == irqNum) {
            kprintf("Checking device %X:%X, status: 0x%X\n", pciDevice->VendorId, pciDevice->DeviceId, pci_config_read_word(pciDevice, PCI_REG_STATUS));
            if ((pciDevice->InterruptHandler != NULL) && pciDevice->InterruptHandler(pciDevice))
                return true;
        }

        // Move to next device.
        pciDevice = pciDevice->Next;
    }
}

uint32_t pci_config_read_dword(pci_device_t *pciDevice, uint8_t reg) {
    // Build address.
    uint32_t address = PCI_PORT_ENABLE_BIT | ((uint32_t)pciDevice->Bus << 16)
        | ((uint32_t)pciDevice->Device << 11) | ((uint32_t)pciDevice->Function << 8) | (reg & 0xFC);

    // Send address to PCI system.
    outl(PCI_PORT_ADDRESS, address);

    // Read 32-bit data value from PCI system.
    return inl(PCI_PORT_DATA);
}

uint16_t pci_config_read_word(pci_device_t *pciDevice, uint8_t reg) {
    // Read 16-bit value.
    return (uint16_t)(pci_config_read_dword(pciDevice, reg) >> ((reg & 0x2) * 8) & 0xFFFF);
}

uint8_t pci_config_read_byte(pci_device_t *pciDevice, uint8_t reg) {
    // Read 8-bit value.
    return (uint8_t)(pci_config_read_dword(pciDevice, reg) >> ((reg & 0x3) * 8) & 0xFF);
}

void pci_config_write_dword(pci_device_t *pciDevice, uint8_t reg, uint32_t value) {
    // Build address.
    uint32_t address = PCI_PORT_ENABLE_BIT | ((uint32_t)pciDevice->Bus << 16)
        | ((uint32_t)pciDevice->Device << 11) | ((uint32_t)pciDevice->Function << 8) | (reg & 0xFC);

    // Send address to PCI system.
    outl(PCI_PORT_ADDRESS, address);

    // Write 32-bit value to PCI system.
    outl(PCI_PORT_DATA, value);
}

void pci_config_write_word(pci_device_t *pciDevice, uint8_t reg, uint16_t value) {
    // Get current 32-bit value.
    uint32_t currentValue = pci_config_read_dword(pciDevice, reg);

    // Replace portion of the 32-bit value with the new 16-bit value.
    uint32_t newValue = ((uint32_t)value << ((reg & 0x2) * 8)) | (currentValue & (0xFFFF0000 >> (reg & 0x2) * 8));

    // Write the resulting 32-bit value.
    pci_config_write_dword(pciDevice, reg, newValue);
}

void pci_config_write_byte(pci_device_t *pciDevice, uint8_t reg, uint8_t value) {
    // Get current 16-bit value.
    uint16_t currentValue = pci_config_read_word(pciDevice, reg);

    // Replace portion of 16-bit value with the new 8-bit value.
    uint16_t newValue = ((uint16_t)value << ((reg & 0x1) * 8)) | (currentValue & (0xFF00 >> (reg & 0x1) * 8));

    // Write the resulting 16-bit value.
    pci_config_write_word(pciDevice, reg, newValue);
}

void pci_enable_busmaster(pci_device_t *pciDevice) {
    // Set busmaster bit.
    pci_config_write_word(pciDevice, PCI_REG_COMMAND, pci_config_read_word(pciDevice, PCI_REG_COMMAND) | PCI_CMD_BUSMASTER);
}

/**
 * Print the description for a PCI device
 * @param dev PCIDevice struct with PCI device info
 */
void pci_print_info(pci_device_t *pciDevice) {
    // Print base info
    kprintf("\e[94mPCI device: %4X:%4X (%4X:%4X) | Class %X Sub %X | Bus %d Device %d Function %d\n", 
        pciDevice->VendorId, pciDevice->DeviceId, pciDevice->SubVendorId, pciDevice->SubDeviceId, pciDevice->Class, pciDevice->Subclass, pciDevice->Bus, 
        pciDevice->Device, pciDevice->Function);
    
    // Print class info and base addresses
    kprintf("\e[96m  - %s\n", pci_class_descriptions[pciDevice->Class]);

    // Print base addresses.
    for (uint8_t i = 0; i < PCI_BAR_COUNT; i++)
        if (pciDevice->BaseAddresses[i].BaseAddress)
            kprintf("  - BAR%u: 0x%X (%s)\n", i, pciDevice->BaseAddresses[i].BaseAddress, pciDevice->BaseAddresses[i].PortMapped ? "port-mapped" : "memory-mapped");

    // Interrupt info
    if(pciDevice->InterruptNo != 0) { 
        kprintf("  - Interrupt %u (Pin %u Line %u\e[0m\n", pciDevice->InterruptNo, pciDevice->InterruptPin, pciDevice->InterruptLine);
    }
}

/**
 * Check and get various info about a PCI card
 * @param  bus      PCI bus to read from
 * @param  device   PCI slot to read from
 * @param  function PCI card function to read from
 * @return          A PCIDevice struct with the info filled
 */
pci_device_t *pci_get_device(uint8_t bus, uint8_t device, uint8_t function, ACPI_BUFFER *routingBuffer) {
    // Create temporary PCI device object.
    pci_device_t pciDeviceTemp = { };

    // Set device address.
    pciDeviceTemp.Bus = bus;
    pciDeviceTemp.Device = device;
    pciDeviceTemp.Function = function;

    // Get device vendor. If vendor is 0xFFFF, the device doesn't exist.
    pciDeviceTemp.VendorId = pci_config_read_word(&pciDeviceTemp, PCI_REG_VENDOR_ID);
    if (pciDeviceTemp.VendorId == PCI_DEVICE_VENDOR_NONE)
        return NULL;

    // Create PCI device object.
    pci_device_t *pciDevice = kheap_alloc(sizeof(pci_device_t));
    memset(pciDevice, 0, sizeof(pci_device_t));

    // Set device address.
    pciDevice->Bus = bus;
    pciDevice->Device = device;
    pciDevice->Function = function;

    // Get PCI identification variables.
    pciDevice->VendorId = pci_config_read_word(pciDevice, PCI_REG_VENDOR_ID);
    pciDevice->DeviceId = pci_config_read_word(pciDevice, PCI_REG_DEVICE_ID);
    pciDevice->SubVendorId = pci_config_read_word(pciDevice, PCI_REG_SUB_VENDOR_ID);
    pciDevice->SubDeviceId = pci_config_read_word(pciDevice, PCI_REG_SUB_DEVICE_ID);
    pciDevice->RevisionId = pci_config_read_byte(pciDevice, PCI_REG_REVISION_ID);
    pciDevice->Class = pci_config_read_byte(pciDevice, PCI_REG_CLASS);
    pciDevice->Subclass = pci_config_read_byte(pciDevice, PCI_REG_SUBCLASS);
    pciDevice->Interface = pci_config_read_byte(pciDevice, PCI_REG_PROG_IF);
    pciDevice->HeaderType = pci_config_read_byte(pciDevice, PCI_REG_HEADER_TYPE);

    // Get interrupt info.
    pciDevice->InterruptPin = pci_config_read_byte(pciDevice, PCI_REG_INTERRUPT_PIN);
    pciDevice->InterruptLine = pci_config_read_byte(pciDevice, PCI_REG_INTERRUPT_LINE);

    // Get base address registers.
    for (uint8_t i = 0; i < PCI_BAR_COUNT; i++) {
        // Get BAR.
        uint32_t bar = pci_config_read_dword(pciDevice, PCI_REG_BAR0 + (i * sizeof(uint32_t)));

        // Fill in info.
        pciDevice->BaseAddresses[i].PortMapped = (bar & PCI_BAR_TYPE_PORT) != 0;
        if (pciDevice->BaseAddresses[i].PortMapped) {
            // Port I/O bar.
            pciDevice->BaseAddresses[i].BaseAddress = bar & PCI_BAR_PORT_MASK;
        }
        else {
            pciDevice->BaseAddresses[i].BaseAddress = bar & PCI_BAR_MEMORY_MASK;
            pciDevice->BaseAddresses[i].AddressIs64bits = (bar & PCI_BAR_BITS64) != 0;
            pciDevice->BaseAddresses[i].Prefetchable = (bar & PCI_BAR_PREFETCHABLE) != 0;
        }
    }

    // Find device in ACPI PRT table.
    pciDevice->InterruptNo = pciDevice->InterruptLine;
    if (routingBuffer->Pointer != NULL) {
        ACPI_PCI_ROUTING_TABLE *table = routingBuffer->Pointer;
        while (((uintptr_t)table < (uintptr_t)routingBuffer->Pointer + routingBuffer->Length) && table->Length) {
            // Did we find our device and interrupt pin? ACPI starts at 0, PCI starts at 1.
            if (table->Pin == (pciDevice->InterruptPin - 1) && (table->Address >> 16) == pciDevice->Device) {
                if (table->Source[0]) {
                    kprintf("IRQ: Pin 0x%X, Address 0x%llX, Source: %s, Index: %d\n", table->Pin, table->Address, (char*)table->Source, table->SourceIndex);
                }
                else {
                    kprintf("IRQ: Pin 0x%X, Address 0x%llX, Global interrupt: 0x%X\n", table->Pin, table->Address, table->SourceIndex);
                    pciDevice->InterruptNo = (uint8_t)table->SourceIndex;
                }
                break;
            }        

            // Move to next entry.
            table = (ACPI_PCI_ROUTING_TABLE *)((uintptr_t)table + table->Length);
        }
    }

    if (pciDevice->InterruptNo >= irqs_get_count())
        pciDevice->InterruptNo = 0;

    // Return the device.
    return pciDevice;
}

static void pci_add_device(pci_device_t *pciDevice, pci_device_t *parentPciDevice) {
    pciDevice->Parent = parentPciDevice;
    if (PciDevices != NULL) {
        pci_device_t *lastDevice = PciDevices;
        while (lastDevice->Next != NULL)
            lastDevice = lastDevice->Next;
        lastDevice->Next = pciDevice;
    }
    else
        PciDevices = pciDevice;

    // Enable interrupt.
    if ((pciDevice->InterruptNo > 0) && !(irqs_handler_mapped(pciDevice->InterruptNo, pci_irq_callback))) {
        // Open up the interrupt in the APIC if needed.
        if (pciDevice->InterruptNo >= IRQ_ISA_COUNT)
            ioapic_enable_interrupt_pci(ioapic_remap_interrupt(pciDevice->InterruptNo), IRQ_OFFSET + pciDevice->InterruptNo);

        // Install our PCI handler for the IRQ. This handler calls device handlers as required.
        irqs_install_handler(pciDevice->InterruptNo, pci_irq_callback);
    }
}

/**
 * Check a bus for PCI devices
 * @param bus The bus number to scan
 */
static void pci_check_busses(uint8_t bus, pci_device_t *parentPciDevice) {
    ACPI_BUFFER buffer = {};
    uint8_t parentDevice = 0;
    if (parentPciDevice != NULL)
        parentDevice = parentPciDevice->Device;
    kprintf("Getting _PRT for bus %u on device %u...\n", bus, parentDevice);
    ACPI_STATUS status = acpi_get_prt(parentDevice << 16, &buffer);
    if (status) {
        kprintf("PCI: An error occurred getting the IRQ routing table: 0x%X!\n", status);
        if (buffer.Pointer)
            ACPI_FREE(buffer.Pointer);
        buffer.Pointer = NULL;
    }

    // Check each device on bus
    kprintf("PCI: Enumerating devices on bus %u...\n", bus);
    for (uint8_t device = 0; device < PCI_NUM_DEVICES; device++) {
        // Get info on the device and print info
        pci_device_t *pciDevice = pci_get_device(bus, device, 0, &buffer);
        if (pciDevice == NULL)
            continue;

        // Add device.
        pci_print_info(pciDevice);
        pci_add_device(pciDevice, parentPciDevice);

        // If the card reports more than one function, let's scan those too.
        if ((pciDevice->HeaderType & PCI_HEADER_TYPE_MULTIFUNC) != 0) {
            kprintf("\e[32m  - Scanning other functions on multifunction device!\e[0m\n");
            // Check each function on the device
            for (uint8_t func = 1; func < PCI_NUM_FUNCTIONS; func++) {
                pci_device_t *funcDevice = pci_get_device(bus, device, func, &buffer);
                if (funcDevice == NULL)
                    continue;

                // Add device.
                pci_print_info(funcDevice);
                pci_add_device(funcDevice, parentPciDevice);
            }
        }

        // Is the device a bridge?
        if (pciDevice->Class == PCI_CLASS_BRIDGE && pciDevice->Subclass == PCI_SUBCLASS_BRIDGE_PCI) {
            uint16_t secondaryBus = pci_config_read_word(pciDevice, PCI_REG_BAR2);
            uint16_t primaryBus = (secondaryBus & ~0xFF00);
            secondaryBus = (secondaryBus & ~0x00FF) >> 8;
            kprintf("\e[32m  - PCI bridge, Primary %X Secondary %X, scanning now.\e[0m\n", primaryBus, secondaryBus);
            pci_check_busses(secondaryBus, pciDevice);

        // If device is a different kind of bridge
        } else if (pciDevice->Class == PCI_CLASS_BRIDGE) {
            kprintf("\e[91m  - Ignoring non-PCI bridge\e[0m\n");
        }
    }

    // Free interrupt table.
    if (buffer.Pointer)
        ACPI_FREE(buffer.Pointer);
}

static void pci_load_drivers(void) {
    // Run through devices and load drivers.
    kprintf("PCI: Loading drivers for devices...\n");
    pci_device_t *pciDevice = PciDevices;
    while (pciDevice != NULL) {
        pci_print_info(pciDevice);

        // Attempt to find driver.
        uint16_t driverIndex = 0;
        while (PciDrivers[driverIndex].Initialize != NULL) {
            if (PciDrivers[driverIndex].Initialize(pciDevice))
                break;
            driverIndex++;
        }

        // Move to next device.
        pciDevice = pciDevice->Next;
    }
}

/**
 * Set various variables the PCI driver uses
 */
void pci_init(void) {
    pci_class_descriptions[PCI_CLASS_NONE] = "Unclassified device";
    pci_class_descriptions[PCI_CLASS_MASS_STORAGE] = "Mass storage controller";
    pci_class_descriptions[PCI_CLASS_NETWORK] = "Network controller";
    pci_class_descriptions[PCI_CLASS_DISPLAY] = "Display controller";
    pci_class_descriptions[PCI_CLASS_MULTIMEDIA] = "Multimedia controller";
    pci_class_descriptions[PCI_CLASS_MEMORY] = "Memory controller";
    pci_class_descriptions[PCI_CLASS_BRIDGE] = "Bridge";
    pci_class_descriptions[PCI_CLASS_SIMPLE_COMM] = "Communication controller";
    pci_class_descriptions[PCI_CLASS_BASE_SYSTEM] = "Generic system peripheral";
    pci_class_descriptions[PCI_CLASS_INPUT] = "Input device controller";
    pci_class_descriptions[PCI_CLASS_DOCKING] = "Docking station";
    pci_class_descriptions[PCI_CLASS_PROCESSOR] = "Processor";
    pci_class_descriptions[PCI_CLASS_SERIAL_BUS] = "Serial bus controller";
    pci_class_descriptions[PCI_CLASS_WIRELESS] = "Wireless controller";
    pci_class_descriptions[PCI_CLASS_INTELLIGENT_IO] = "Intelligent controller";
    pci_class_descriptions[PCI_CLASS_SATELLITE_COMM] = "Satellite communications controller";
    pci_class_descriptions[PCI_CLASS_ENCRYPTION] = "Encryption controller";
    pci_class_descriptions[PCI_CLASS_DATA_ACQUISITION] = "Signal processing controller";
    pci_class_descriptions[0x12] = "Processing accelerators";
    pci_class_descriptions[0x13] = "Non-Essential Instrumentation";
    pci_class_descriptions[0x40] = "Coprocessor";
    pci_class_descriptions[PCI_CLASS_UNKNOWN] = "Unassigned class";

    // Begin scanning at bus 0.
    pci_check_busses(0, NULL);

    // Load drivers for devices.
	pci_load_drivers();
}
