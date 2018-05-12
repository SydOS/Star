/*
 * File: rtl8139.c
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
#include <tools.h>
#include <driver/pci.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/interrupts/irqs.h>

//extern void _irq16();
//extern void _irq17();
//extern void _irq18();
//extern void _irq19();

/*void test_handler(IrqRegisters_t* regs, uint8_t irq) {
kprintf_nlock("Test: APIC INT %d\n", irq);

	// Send EOI
    lapic_eoi();
}*/

struct RTL8139 {
	bool UsesEEPROM;

	uint32_t BaseAddress;

	uint8_t MACAddress[6];
};

bool rtl_callback(pci_device_t *dev) {
	if (!(dev->VendorId == 0x10EC && dev->DeviceId == 0x8139)) {
        return false;
    }

	struct RTL8139 *rtl = (struct RTL8139*)dev->DriverObject;
	outw(rtl->BaseAddress + 0x3E, 0xFFFF);
	kprintf("RTL8139: Cleared interrupt\n");
	return true;
}

bool rtl8139_init(pci_device_t* dev) {
	// Is the PCI device an RTL8139?
    if (!(dev->VendorId == 0x10EC && dev->DeviceId == 0x8139)) {
        return false;
    }

	// Allocate RTL8139 struct
	struct RTL8139 *rtl = (struct RTL8139*)kheap_alloc(sizeof(struct RTL8139));
	dev->DriverObject = rtl;
	kprintf("RTL8139: Pointed RTL struct to DriverObject\n");
	dev->InterruptHandler = rtl_callback;
	kprintf("RTL8139: Pointed RTL callback handler function to InterruptHandler\n");

	// Setup base address
	for(uint8_t i = 0; i < 6; i++)
		if ((rtl->BaseAddress = dev->BaseAddresses[i].BaseAddress) != 0) break;

	// Check base address is valid
	if(rtl->BaseAddress == 0) {
		kprintf("RTL8139: INVALID BAR\n");
		return false;
	}

	kprintf("RTL8139: using BAR 0x%X\n", rtl->BaseAddress);

// bus master
//		uint16_t cmd = pci_config_read_word(dev, PCI_REG_COMMAND);
//	pci_config_write_word(dev, PCI_REG_COMMAND, cmd | 0x04);
//	cmd = pci_config_read_word(dev, PCI_REG_COMMAND);

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

	// reset.
	outb(rtl->BaseAddress + 0x37, 0x10);
	while ((inb(rtl->BaseAddress + 0x37) & 0x10) != 0);
	kprintf("RTL8139: Reset card\n");

	uintptr_t recDma = 0;
	pmm_dma_get_free_frame(&recDma);
	kprintf("RTL8139: Allocated DMA buffer\n");

	outl(rtl->BaseAddress + 0x30, recDma - memInfo.kernelVirtualOffset);
	outw(rtl->BaseAddress + 0x3C, 0xFFFF);
	outl(rtl->BaseAddress + 0x44, 0xF | (1 << 7));
	kprintf("RTL8139: Transmitted DMA buffer location to card\n");

	outb(rtl->BaseAddress + 0x37, 0x0C);

	kprintf("RTL8139: Media statudsfsd: 0x%X\n", inb(rtl->BaseAddress + 0x58));

	return true;
}