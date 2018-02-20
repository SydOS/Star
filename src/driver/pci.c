#include <main.h>

void pci_check_busses() {
	// Check each bus
	for (uint16_t bus = 0; bus < 256; bus++) {
		// Check each device on bus
		for (uint8_t device; device < 32; device++) {
			pci_check_device(bus, device);
		}
	}
}