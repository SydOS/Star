#include <main.h>
#include <io.h>
#include <kprint.h>
#include <kernel/kheap.h>
#include <driver/pci.h>

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

void pci_check_device(uint8_t bus, uint8_t device) {
	uint16_t vendorID = pci_config_read_word(bus,device,0,PCI_VENDOR_ID);
	uint16_t deviceID = pci_config_read_word(bus,device,0,PCI_DEVICE_ID);
	uint16_t classInfo = pci_config_read_word(bus,device,0,0xA);
	if (vendorID == 0xFFFF) return;

    struct PCIDevice *this_device = (struct PCIDevice*)kheap_alloc(sizeof(struct PCIDevice));
    this_device->VendorID = vendorID;
    this_device->DeviceID = deviceID;
    this_device->Class = (classInfo & ~0x00FF) >> 8;
    this_device->Subclass = (classInfo & ~0xFF00);

	kprintf("PCI device: %X:%X | Class %X Sub %X | Bus %d Device %d\n", 
        this_device->VendorID, this_device->DeviceID, this_device->Class, 
        this_device->Subclass, bus, device);
    
    kheap_free(this_device);
}

void pci_check_busses() {
	// Check each bus
	for (uint16_t bus = 0; bus < 256; bus++) {
		// Check each device on bus
		for (uint8_t device = 0; device < 32; device++) {
			pci_check_device(bus, device);
		}
	}
}