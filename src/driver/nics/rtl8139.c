#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/pci.h>
#include <kernel/memory/kheap.h>

struct RTL8139 {
	bool UsesEEPROM;

	uint32_t BaseAddress;

	uint8_t MACAddress[6];
};

void rtl8139_init(struct PCIDevice* dev) {
	// Allocate RTL8139 struct
	struct RTL8139 *rtl = (struct RTL8139*)kheap_alloc(sizeof(struct RTL8139));

	// Setup base address
	for(uint8_t i = 0; i < 6; i++)
		if ((rtl->BaseAddress = dev->BAR[i]) != 0) break;

	rtl->BaseAddress = rtl->BaseAddress - 1;

	// Check base address is valid
	if(rtl->BaseAddress == 0) {
		kprintf("RTL8139: INVALID BAR\n");
		return;
	}

	kprintf("RTL8139: using BAR 0x%X\n", rtl->BaseAddress);

	// Bring card out of low power mode
	outb(rtl->BaseAddress + 0x52, 0x00);
	kprintf("RTL8139: brought card out of low power mode\n");

	// Read MAC address from RTL8139
	for (uint8_t i = 0; i < 6; i++) {
		rtl->MACAddress[i] = inb(rtl->BaseAddress + i);
	}
	kprintf("RTL8139: MAC %X:%X:%X:%X:%X:%X\n", rtl->MACAddress[0], rtl->MACAddress[1],
		rtl->MACAddress[2], rtl->MACAddress[3], rtl->MACAddress[4],
		rtl->MACAddress[5]);

	// Free device for now
	kheap_free(rtl);
}