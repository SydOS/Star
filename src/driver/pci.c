#include <main.h>
#include <io.h>
#include <kprint.h>


#define PCI_VENDOR_ID            0x00 // 2
#define PCI_DEVICE_ID            0x02 // 2
#define PCI_COMMAND              0x04 // 2
#define PCI_STATUS               0x06 // 2
#define PCI_REVISION_ID          0x08 // 1

#define PCI_PROG_IF              0x09 // 1
#define PCI_SUBCLASS             0x0A // 1
#define PCI_CLASS                0x0B // 1
#define PCI_CACHE_LINE_SIZE      0x0C // 1
#define PCI_LATENCY_TIMER        0x0D // 1
#define PCI_HEADER_TYPE          0x0E // 1
#define PCI_BIST                 0x0F // 1
#define PCI_BAR0                 0x10 // 4
#define PCI_BAR1                 0x14 // 4
#define PCI_BAR2                 0x18 // 4
#define PCI_BAR3                 0x1C // 4
#define PCI_BAR4                 0x20 // 4
#define PCI_BAR5 0x24 // 4

#define PCI_ADDRESS_PORT 0xCF8
#define PCI_VALUE_PORT 0xCFC


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

uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outb(PCI_ADDRESS_PORT, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = inb(PCI_VALUE_PORT + (offset & 3));
    return (tmp);
}

void pci_check_device(uint8_t bus, uint8_t device) {
	uint16_t vendorID = pci_config_read_word(bus,device,0,PCI_VENDOR_ID);
	uint16_t deviceID = pci_config_read_word(bus,device,0,PCI_DEVICE_ID);
	uint8_t class = pci_config_read_byte(bus,device,0,PCI_CLASS);
	uint8_t subclass = pci_config_read_byte(bus,device,0,PCI_SUBCLASS);
	if (vendorID == 0xFFFF) return;
	kprintf("PCI device: %X:%X | Class %X Sub %X\n", vendorID, deviceID, class, subclass);
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