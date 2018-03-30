#include <main.h>
#include <io.h>
#include <kprint.h>
#include <kernel/memory/kheap.h>
#include <driver/vga.h>
#include <driver/pci.h>
#include <kernel/interrupts/irqs.h>

#include <acpi.h>

static void pci_irq_callback(IrqRegisters_t *regs, uint8_t irqNum) {
    kprintf_nlock("IRQ %u raised!\n", irqNum);
}

/**
 * Print the description for a PCI device
 * @param dev PCIDevice struct with PCI device info
 */
void pci_print_info(struct PCIDevice* dev) {
    // Print base info
    vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprintf("PCI device: %X:%X | Class %X Sub %X | Bus %d Device %d Function %d\n", 
        dev->VendorID, dev->DeviceID, dev->Class,  dev->Subclass, dev->Bus, 
        dev->Device, dev->Function);
    
    // Print class info and base addresses
    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("  - %s\n", pci_class_descriptions[dev->Class]);

    // Print PCIDevice's base addresses if they exist
    if(dev->BAR[0] != 0 || dev->BAR[1] != 0 || dev->BAR[2] != 0 ||
       dev->BAR[3] != 0 || dev->BAR[4] != 0 || dev->BAR[5] != 0) {
        kprintf("  - BARS: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", dev->BAR[0], 
            dev->BAR[1], dev->BAR[2], dev->BAR[3], dev->BAR[4], dev->BAR[5]);
    }

    // Interrupt info
    if(dev->IntPIN != 0) { 
        kprintf("  - Interrupt PIN %d Line %d\n", dev->IntPIN, dev->IntLine);
    }
}

/**
 * Read a word from the PCI config area
 * @param  dev    PCIDevice struct with at least the bus, device, and function
 *                filled.
 * @param  offset Offset to use for the PCI config address
 * @return        A uint16_t containing the data returned by the card
 */
uint16_t pci_config_read_word(struct PCIDevice* dev, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)dev->Bus;
    uint32_t lslot = (uint32_t)dev->Device;
    uint32_t lfunc = (uint32_t)dev->Function;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(PCI_ADDRESS_PORT, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl(PCI_VALUE_PORT) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

void pci_config_write_word(struct PCIDevice* dev, uint8_t offset, uint16_t data) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)dev->Bus;
    uint32_t lslot = (uint32_t)dev->Device;
    uint32_t lfunc = (uint32_t)dev->Function;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(PCI_ADDRESS_PORT, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    outw(PCI_VALUE_PORT + (offset & 0x3), data);
}

/**
 * Read a DWORD (32 bits) from a PCIDevice's config space. Quite hacky, but
 * it works
 * @param  dev    PCIDevice struct with at least the bus, device, and function
 *                filled.
 * @param  offset Offset to use for the PCI config address
 * @return        A uint32_t containing the data returned by the card
 */
uint32_t pci_config_read_dword(struct PCIDevice* dev, uint8_t offset) {
    uint32_t result;
    result = pci_config_read_word(dev, offset) + (pci_config_read_word(dev, offset+2) << 16);
    return result;
}

/**
 * Check and get various info about a PCI card
 * @param  bus      PCI bus to read from
 * @param  device   PCI slot to read from
 * @param  function PCI card function to read from
 * @return          A PCIDevice struct with the info filled
 */
struct PCIDevice* pci_check_device(uint8_t bus, uint8_t device, uint8_t function, ACPI_BUFFER *routingBuffer) {
    // Define our PCIDevice
    struct PCIDevice *this_device = (struct PCIDevice*)kheap_alloc(sizeof(struct PCIDevice));

    // Set base configuration address and other base info for our PCIDevice
    this_device->Bus = bus;
    this_device->Device = device;
    this_device->Function = function;

    // Define variables inside our PCIDevice struct
    // Class and Subclass are bytes, but we read a word, so we have to split the
    // two halves
    this_device->VendorID = pci_config_read_word(this_device,PCI_VENDOR_ID);
    this_device->DeviceID = pci_config_read_word(this_device,PCI_DEVICE_ID);
    uint16_t classInfo = pci_config_read_word(this_device,PCI_SUBCLASS);
    this_device->Class = (classInfo & ~0x00FF) >> 8;
    this_device->Subclass = (classInfo & ~0xFF00);
    this_device->HeaderType = pci_config_read_word(this_device,PCI_HEADER_TYPE);

    // Define interrupt info in our PCIDevice struct
    uint16_t intInfo = pci_config_read_word(this_device,PCI_INT_LINE);
    this_device->IntPIN = (intInfo & ~0x00FF) >> 8;
    this_device->IntLine = (intInfo & ~0xFF00);

    // Read all 6 base register addresses and store in PCIDevice struct
    for(int i = 0; i < 6; i++) {
        this_device->BAR[i] = pci_config_read_dword(this_device, PCI_BAR0 + (i*4));
    }

    // Return if vendor is none
    if (this_device->VendorID == 0xFFFF) return this_device;

    // Find device in ACPI PRT table.
    ACPI_PCI_ROUTING_TABLE *table = routingBuffer->Pointer;
    while (((uintptr_t)table < (uintptr_t)routingBuffer->Pointer + routingBuffer->Length) && table->Length) {
        // Did we find our device and interrupt pin? ACPI starts at 0, PCI starts at 1.
        if (table->Pin == (this_device->IntPIN - 1) && (table->Address >> 16) == this_device->Device) {
            if (table->Source[0]) {
                kprintf("IRQ: Pin 0x%X, Address 0x%llX, Source: %s, Index: %d\n", table->Pin, table->Address, (char*)table->Source, table->SourceIndex);
            }
            else {
                kprintf("IRQ: Pin 0x%X, Address 0x%llX, Global interrupt: 0x%X\n", table->Pin, table->Address, table->SourceIndex);
                this_device->apicLine = (uint8_t)table->SourceIndex;
                irqs_install_handler(this_device->apicLine, pci_irq_callback);
            }
            break;
        }        

        // Move to next entry.
        table = (uintptr_t)table + table->Length;
    }

    pci_print_info(this_device);

    // If the card reports more than one function, let's scan those too
    if((this_device->HeaderType & 0x80) != 0 && function == 0) {
        vga_setcolor(VGA_COLOR_GREEN, VGA_COLOR_BLACK); 
        kprintf("  - Scanning other functions on multifunction device!\n");
        // Check each function on the device
        for (int t_function = 1; t_function < 8; t_function++) {
            struct PCIDevice *func_device = pci_check_device(bus, device, t_function, routingBuffer);
            kheap_free(func_device);
        }
    }
    
    // Return the device
    return this_device;
}

/**
 * Check a bus for PCI devices
 * @param bus The bus number to scan
 */
void pci_check_busses(uint8_t bus, uint8_t device) {
    ACPI_BUFFER buffer;
    kprintf("Getting _PRT for bus %d on device %d...\n", bus ,device);
    ACPI_STATUS status = acpi_get_prt(device << 16, &buffer);

    // Check each device on bus
    for (int device = 0; device < 32; device++) {
        // Get info on the device and print info
        struct PCIDevice *this_device = pci_check_device(bus, device, 0, &buffer);

        // Check if the device is the RTL8139
        if(this_device->VendorID == 0x10EC && this_device->DeviceID == 0x8139) {
            vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            kprintf("  - DETECTED RTL8139\n");
            rtl8139_init(this_device);
        }
        
        // If device is a PCI bridhge
        if(this_device->Class == 6 && this_device->Subclass == 4) {
            vga_setcolor(VGA_COLOR_GREEN, VGA_COLOR_BLACK); 
            uint16_t seconaryBus = pci_config_read_word(this_device,PCI_BAR2);
            uint16_t primaryBus = (seconaryBus & ~0xFF00);
            seconaryBus = (seconaryBus & ~0x00FF) >> 8;
            kprintf("  - PCI bridge, Primary %X Seconary %X, scanning now.\n", primaryBus, seconaryBus);
            pci_check_busses(seconaryBus, device);

        // If device is a different kind of bridge
        } else if(this_device->Class == 6) {
            vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK); 
            kprintf("  - Ignoring non-PCI bridge\n");
        }

        // Free object
        kheap_free(this_device);
    }

    // Free interrupt table.
    if (buffer.Pointer)
        ACPI_FREE(buffer.Pointer);

    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/**
 * Set various variables the PCI driver uses
 */
void pci_init() {
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
}