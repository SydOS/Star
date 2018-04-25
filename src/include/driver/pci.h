#ifndef PCI_H
#define PCI_H

#include <main.h>
#include <driver/pci_classes.h>
#include <kernel/interrupts/irqs.h>

#define PCI_NUM_DEVICES             32
#define PCI_NUM_FUNCTIONS           8

// PCI configuration space registers.
#define PCI_REG_VENDOR_ID           0x00 // 2
#define PCI_REG_DEVICE_ID           0x02 // 2
#define PCI_REG_COMMAND             0x04 // 2
#define PCI_REG_STATUS              0x06 // 2
#define PCI_REG_REVISION_ID         0x08 // 1
#define PCI_REG_PROG_IF             0x09 // 1
#define PCI_REG_SUBCLASS            0x0A // 1
#define PCI_REG_CLASS               0x0B // 1
#define PCI_REG_CACHE_LINE_SIZE		0x0C // 1
#define PCI_REG_LATENCY_TIMER       0x0D // 1
#define PCI_REG_HEADER_TYPE         0x0E // 1
#define PCI_REG_BIST                0x0F // 1

#define PCI_REG_BAR0                0x10 // 4
#define PCI_REG_BAR1                0x14 // 4
#define PCI_REG_BAR2                0x18 // 4
#define PCI_REG_BAR3                0x1C // 4

#define PCI_REG_BAR4                0x20 // 4
#define PCI_REG_BAR5                0x24 // 4
#define PCI_REG_CARDBUS_CIS			0x28 // 4
#define PCI_REG_SUB_VENDOR_ID		0x2C // 2
#define PCI_REG_SUB_DEVICE_ID		0x2E // 2

#define PCI_REG_EXPANSION_ROM		0x30 // 4
#define PCI_REG_CAPABILITIES		0x34 // 1
#define PCI_REG_INTERRUPT_LINE      0x3C // 1
#define PCI_REG_INTERRUPT_PIN       0x3D // 1
#define PCI_REG_MIN_GNT				0x3E // 1
#define PCI_REG_MAX_LAT				0x3F // 1

#define PCI_PORT_ADDRESS 			0xCF8
#define PCI_PORT_DATA				0xCFC
#define PCI_PORT_ENABLE_BIT			0x80000000

#define PCI_DEVICE_VENDOR_NONE		0xFFFF
#define PCI_HEADER_TYPE_MULTIFUNC	0x80
#define PCI_BAR_COUNT				6

#define PCI_BAR_MEMORY_MASK			0xFFFFFFF0
#define PCI_BAR_PORT_MASK			0xFFFFFFFC

#define PCI_BAR_TYPE_MEMORY			0x0
#define PCI_BAR_TYPE_PORT			0x1

#define PCI_BAR_BITS32				0x0
#define PCI_BAR_BITS64				0x4

#define PCI_BAR_PREFETCHABLE		0x8

typedef struct {
    bool PortMapped;
    bool AddressIs64bits;
    bool Prefetchable;

    uint32_t BaseAddress;
} pci_base_register_t;

// PCI device structure.
typedef struct pci_device_t {
    // Relations to other devices.
    struct pci_device_t *Parent;
    struct pci_device_t *Next;

    uintptr_t ConfigurationAddress;
    uint8_t Bus;
    uint8_t Device;
    uint8_t Function;

    uint16_t VendorId;
    uint16_t DeviceId;
    uint16_t SubVendorId;
    uint16_t SubDeviceId;

    uint8_t RevisionId;
    uint8_t Class;
    uint8_t Subclass;
    uint8_t Interface;
    uint8_t HeaderType;

    pci_base_register_t BaseAddresses[PCI_BAR_COUNT];

    uint8_t InterruptPin;
    uint8_t InterruptLine;
    uint8_t InterruptApic;

    // Interrupt handler.
    void *InterruptHandler;
    void *DriverObject;
} pci_device_t;

// Driver object.
typedef struct {
    char *Name;

    bool (*Initialize)(pci_device_t *pciDevice);
} pci_driver_t;

// Driver array.
extern const pci_driver_t PciDrivers[];

typedef bool (*pci_handler)(pci_device_t *pciDevice);

char* pci_class_descriptions[255];

extern uint32_t pci_config_read_dword(pci_device_t *pciDevice, uint8_t reg);
extern uint16_t pci_config_read_word(pci_device_t *pciDevice, uint8_t reg);
extern uint8_t pci_config_read_byte(pci_device_t *pciDevice, uint8_t reg);
extern void pci_config_write_dword(pci_device_t *pciDevice, uint8_t reg, uint32_t value);
extern void pci_config_write_word(pci_device_t *pciDevice, uint8_t reg, uint16_t value);
extern void pci_config_write_byte(pci_device_t *pciDevice, uint8_t reg, uint8_t value);

extern void pci_init(void);

#endif
