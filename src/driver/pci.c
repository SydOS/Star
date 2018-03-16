#include <main.h>
#include <io.h>
#include <kprint.h>
#include <kernel/memory/kheap.h>
#include <driver/vga.h>
#include <driver/pci.h>

/**
 * Print the description for a PCI device
 * @param dev PCIDevice struct with PCI device info
 */
void pci_print_info(struct PCIDevice* dev) {
    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("  - ");
    kprintf(pci_class_descriptions[dev->Class]);
    kprintf("\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/**
 * Read a word from the PCI config area
 * @param  bus    PCI bus to read from
 * @param  slot   PCI slot to read from
 * @param  func   PCI card function to read from
 * @param  offset PCI card's config area offset
 * @return        A uint16_t containing the data returned by the card
 */
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
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

/**
 * Check and get various info about a PCI card
 * @param  bus      PCI bus to read from
 * @param  device   PCI slot to read from
 * @param  function PCI card function to read from
 * @return          A PCIDevice struct with the info filled
 */
struct PCIDevice* pci_check_device(uint8_t bus, uint8_t device, uint8_t function) {
    // Get various bits of info from the PCI card
    uint16_t vendorID = pci_config_read_word(bus,device,0,PCI_VENDOR_ID);
    uint16_t deviceID = pci_config_read_word(bus,device,0,PCI_DEVICE_ID);
    uint16_t classInfo = pci_config_read_word(bus,device,0,PCI_SUBCLASS);
    uint16_t headerType = (pci_config_read_word(bus,device,0,PCI_HEADER_TYPE) & ~0x00FF) >> 8;

    // Define our PCIDevice
    struct PCIDevice *this_device = (struct PCIDevice*)kheap_alloc(sizeof(struct PCIDevice));
    
    // Define variables inside our PCIDevice struct
    // Class and Subclass are bytes, but we read a word, so we have to split the
    // two halves
    this_device->VendorID = vendorID;
    this_device->DeviceID = deviceID;
    this_device->Class = (classInfo & ~0x00FF) >> 8;
    this_device->Subclass = (classInfo & ~0xFF00);
    this_device->HeaderType = headerType;

    // Return if vendor is none
    if (vendorID == 0xFFFF) return this_device;

    // Print info about the card
    vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprintf("PCI device: %X:%X | Class %X Sub %X | Bus %d Device %d Function %d\n", 
        this_device->VendorID, this_device->DeviceID, this_device->Class, 
        this_device->Subclass, bus, device, function);
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    // If the card reports more than one function, let's scan those too
    if((this_device->HeaderType & 0x80) != 0) {
        struct PCIDevice *sub_device = pci_check_device(bus, device, function++);
        kheap_free(sub_device);
    }
    
    // Return the device
    return this_device;
}

/**
 * Check a bus for PCI devices
 * @param bus The bus number to scan
 */
void pci_check_busses(uint8_t bus) {
    // Define some initial variables
    uint8_t device = 0;
    bool onlyOneBus = true;

    // If our bus is zero, check some things
    if(bus == 0) {
        // Set the device to one to skip this device (already being checked) in
        // allover scan
        device = 1;

        // Scan the very first PCI device, it's the host controller, then print
        // info
        struct PCIDevice *host_device = pci_check_device(0, 0, 0);
        pci_print_info(host_device);

        // If the header states more than 1 function (0 index) we have multiple
        // busses. Otherwise, set to false to skip checking busses
        if((host_device->HeaderType & 0x80) != 0) {
            onlyOneBus = false;
        }

        // Free object
        kheap_free(host_device);
    }

    // Check each device on bus
    for (; device < 32; device++) {
        // Get info on the device and print info
        struct PCIDevice *this_device = pci_check_device(bus, device, 0);
        if(this_device->VendorID != 0xFFFF) {
            pci_print_info(this_device);
        }
        
        // Check if we have more than one bus
        if(onlyOneBus == false) {
            // If device is a PCI bridhge
            if(this_device->Class == 6 && this_device->Subclass == 4) {
                vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK); 
                kprintf("Detected PCI bridge, but cannot reach it yet\n");
                vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

            // If device is a different kind of bridge
            } else if(this_device->Class == 6) {
                vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK); 
                kprintf("Detected other type of bridge on PCI bus. No other protocols implemented yet...\n");
                vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            }
        }

        // Free object
        kheap_free(this_device);
    }
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