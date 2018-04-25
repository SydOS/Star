#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/pci.h>

#include <kernel/memory/kheap.h>
#include <driver/vga.h>

#include <kernel/interrupts/irqs.h>

#include <acpi.h>

// Array of PCI device drivers.
const pci_driver_t PciDrivers[] = {


    // End driver.
    { "", NULL }
};

// PCI devices.
pci_device_t *StartPciDevice = NULL;

static pci_device_t *pciDevices = NULL;
static size_t pciDevicesLength = 0;

static void pci_irq_callback(IrqRegisters_t *regs, uint8_t irqNum) {
    kprintf_nlock("PCI: IRQ %u raised!\n", irqNum);

    // Search through PCI devices until we find the device that raised the interrupt.
    for (uint16_t i = 0; i < pciDevicesLength; i++) {
        // Get pointer to device.
        pci_device_t *device = &pciDevices[i];

        // Does the device match the IRQ raised?
        if (device->InterruptLine == irqNum) {
            kprintf_nlock("%X:%X status: 0x%X\n", device->VendorId, device->DeviceId, pci_config_read_word(device, PCI_REG_STATUS));

            // Call any handler registered.
            pci_handler handler = (pci_handler)device->InterruptHandler;
            if (handler)
                if (handler(device))
                    return;
        }
    }
}

static void pci_install_irq_handler_apic(uint8_t irq) {
    if (!(irqs_handler_mapped(irq))) {
        // Map to APIC and add handler.
        ioapic_enable_interrupt_pci(ioapic_remap_interrupt(irq), IRQ_OFFSET + irq);
        irqs_install_handler(irq, pci_irq_callback);
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
        | ((uint32_t)pciDevice->Device << 11) | ((uint32_t)pciDevice->Function << 8) | (reg << 2);

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

/**
 * Print the description for a PCI device
 * @param dev PCIDevice struct with PCI device info
 */
void pci_print_info(pci_device_t *pciDevice) {
    // Print base info
    vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprintf("PCI device: %X:%X (%X:%X) | Class %X Sub %X | Bus %d Device %d Function %d\n", 
        pciDevice->VendorId, pciDevice->DeviceId, pciDevice->SubVendorId, pciDevice->SubDeviceId, pciDevice->Class, pciDevice->Subclass, pciDevice->Bus, 
        pciDevice->Device, pciDevice->Function);
    
    // Print class info and base addresses
    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("  - %s\n", pci_class_descriptions[pciDevice->Class]);

    // Print base addresses.
    kprintf("  - BARS: ");
    for (uint8_t i = 0; i < PCI_BAR_COUNT; i++)
        if (pciDevice->BaseAddresses[i].BaseAddress)
            kprintf(" %u: 0x%X", i + 1, pciDevice->BaseAddresses[i].BaseAddress);
    kprintf("\n");

    // Interrupt info
    if(pciDevice->InterruptPin != 0) { 
        kprintf("  - Interrupt PIN %d Line %d\n", pciDevice->InterruptPin, pciDevice->InterruptLine);
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
                    pciDevice->InterruptLine = (uint8_t)table->SourceIndex;
                    pci_install_irq_handler_apic(pciDevice->InterruptLine);
                }
                break;
            }        

            // Move to next entry.
            table = (uintptr_t)table + table->Length;
        }
    }

    // Return the device.
    return pciDevice;
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
    for (uint8_t device = 0; device < 32; device++) {
        // Get info on the device and print info
        pci_device_t *pciDevice = pci_get_device(bus, device, 0, &buffer);
        if (pciDevice == NULL)
            continue;

        // Add device.
        pciDevice->Parent = parentPciDevice;
        if (StartPciDevice != NULL) {
            pci_device_t *lastDevice = StartPciDevice;
            while (lastDevice->Next != NULL)
                lastDevice = lastDevice->Next;
            lastDevice->Next = pciDevice;
        }
        else
            StartPciDevice = pciDevice;

        // If the card reports more than one function, let's scan those too.
        if ((pciDevice->HeaderType & PCI_HEADER_TYPE_MULTIFUNC) != 0) {
            vga_setcolor(VGA_COLOR_GREEN, VGA_COLOR_BLACK); 
            kprintf("  - Scanning other functions on multifunction device!\n");
            // Check each function on the device
            for (int t_function = 1; t_function < 8; t_function++) {
                pci_device_t *funcDevice = pci_get_device(bus, device, t_function, &buffer);
                if (funcDevice == NULL)
                    continue;

                // Add device.
                pciDevice->Parent = parentPciDevice;
                pci_device_t *lastDevice = StartPciDevice;
                while (lastDevice->Next != NULL)
                    lastDevice = lastDevice->Next;
                lastDevice->Next = funcDevice;
            }
        }

        // Is the device a bridge?
        if (pciDevice->Class == PCI_CLASS_BRIDGE && pciDevice->Subclass == PCI_SUBCLASS_BRIDGE_PCI) {
            vga_setcolor(VGA_COLOR_GREEN, VGA_COLOR_BLACK); 
            uint16_t secondaryBus = pci_config_read_word(pciDevice, PCI_REG_BAR2);
            uint16_t primaryBus = (secondaryBus & ~0xFF00);
            secondaryBus = (secondaryBus & ~0x00FF) >> 8;
            kprintf("  - PCI bridge, Primary %X Secondary %X, scanning now.\n", primaryBus, secondaryBus);
            pci_check_busses(secondaryBus, pciDevice);

        // If device is a different kind of bridge
        } else if (pciDevice->Class == PCI_CLASS_BRIDGE) {
            vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK); 
            kprintf("  - Ignoring non-PCI bridge\n");
        }
    }

    // Free interrupt table.
    if (buffer.Pointer)
        ACPI_FREE(buffer.Pointer);

    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void pci_display() {
    /*for (uint16_t i = 0; i < pciDevicesLength; i++)
    {
        PciDevice *device = pciDevices+i;
        kprintf("Device %u: status: 0x%X\n", i, pci_config_read_word(device, PCI_REG_STATUS));
        pci_print_info(device);

        
    // Check if the device is the RTL8139
        if(device->VendorId == 0x10EC && device->DeviceId == 0x8139) {
            vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("  - DETECTED RTL8139\n");
            rtl8139_init(device);
        }
        else if (device->Class == 1 && device->Subclass == 1) {
            vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("  - DETECTED ATA CONTROLLER\n");
            ata_init(device);
        }
        else if (device->Class == 0x0C && device->Subclass == 0x03 && pci_config_read_byte(device, PCI_REG_PROG_IF) == 0x00) {
            vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("  - DETECTED UHCI USB CONTROLLER\n");
            usb_uhci_init(device);
        }
        else if (device->Class == 0x0C && device->Subclass == 0x03 && pci_config_read_byte(device, PCI_REG_PROG_IF) == 0x10) {
            vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("  - DETECTED OHCI USB CONTROLLER\n");
            usb_ohci_init(device);
        }
    }
    kprintf("%u total PCI devices\n", pciDevicesLength);*/
}

/**
 * Set various variables the PCI driver uses
 */
void pci_init(void) {
    pci_class_descriptions[0x00] = "Unclassified device";
    pci_class_descriptions[0x01] = "Mass storage controller";
    pci_class_descriptions[0x02] = "Network controller";
    pci_class_descriptions[0x03] = "Display controller";
    pci_class_descriptions[0x04] = "Multimedia controller";
    pci_class_descriptions[0x05] = "Memory controller";
    pci_class_descriptions[0x06] = "Bridge";
    pci_class_descriptions[0x07] = "Communication controller";
    pci_class_descriptions[0x08] = "Generic system peripheral";
    pci_class_descriptions[0x09] = "Input device controller";
    pci_class_descriptions[0x0A] = "Docking station";
    pci_class_descriptions[0x0B] = "Processor";
    pci_class_descriptions[0x0C] = "Serial bus controller";
    pci_class_descriptions[0x0D] = "Wireless controller";
    pci_class_descriptions[0x0E] = "Intelligent controller";
    pci_class_descriptions[0x0F] = "Satellite communications controller";
    pci_class_descriptions[0x10] = "Encryption controller";
    pci_class_descriptions[0x11] = "Signal processing controller";
    pci_class_descriptions[0x12] = "Processing accelerators";
    pci_class_descriptions[0x13] = "Non-Essential Instrumentation";
    pci_class_descriptions[0x40] = "Coprocessor";
    pci_class_descriptions[0xFF] = "Unassigned class";

    // Ensure array is reset.
    pciDevices = NULL;
    pciDevicesLength = 0;

    // Begin scanning at bus 0.
    pci_check_busses(0, NULL);
	pci_display();
}