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


struct PCIDevice {
	uint16_t VendorID;
	uint16_t DeviceID;

	uint8_t Class;
	uint8_t Subclass;
	uint8_t HeaderType;

	uint8_t Bus;
	uint8_t Device;
	uint8_t Function;
};

char* pci_class_descriptions[255];

extern void pci_check_busses(uint8_t bus);
extern void pci_init();