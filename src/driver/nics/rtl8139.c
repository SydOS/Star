#include <main.h>
#include <io.h>
#include <kprint.h>
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

bool rtl_callbac(pci_device_t *device) {
	
kprintf("Clearing RTL8139 interrupt\n");
struct RTL8139 *rtl = (struct RTL8139*)device->DriverObject;
outw(rtl->BaseAddress + 0x3E, 0xFFFF);
return true;
	
}

void rtl8139_init(pci_device_t* dev) {
	// Allocate RTL8139 struct
	struct RTL8139 *rtl = (struct RTL8139*)kheap_alloc(sizeof(struct RTL8139));
	dev->DriverObject = rtl;
	dev->InterruptHandler = rtl_callbac;

	// Setup base address
	//for(uint8_t i = 0; i < 6; i++)
	//	if ((rtl->BaseAddress = dev->BAR[i]) != 0) break;

	rtl->BaseAddress = rtl->BaseAddress - 1;

	// Check base address is valid
	if(rtl->BaseAddress == 0) {
		kprintf("RTL8139: INVALID BAR\n");
		return;
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

	uintptr_t recDma = 0;
	pmm_dma_get_free_frame(&recDma);

	outl(rtl->BaseAddress + 0x30, recDma - memInfo.kernelVirtualOffset);
	outw(rtl->BaseAddress + 0x3C, 0xFFFF);
	outl(rtl->BaseAddress + 0x44, 0xF | (1 << 7));
	//interrupts_irq_install_handler(dev->IntLine, rtl_callbac);
	//interrupts_irq_install_handler(4, rtl_callbac);


	//idt_set_gate(IRQ_OFFSET + dev->apicLine, (uintptr_t)_irqT, 0x08, 0x8E);
	//ioapic_enable_interrupt(ioapic_remap_interrupt(dev->apicLine), dev->apicLine);

	//idt_set_gate(IRQ_OFFSET + 16, (uintptr_t)_irq16, 0x08, 0x8E);
	//ioapic_enable_interrupt_pci(ioapic_remap_interrupt(16), IRQ_OFFSET + 16);
	//idt_set_gate(IRQ_OFFSET + 17, (uintptr_t)_irq17, 0x08, 0x8E);
	//ioapic_enable_interrupt_pci(ioapic_remap_interrupt(17), IRQ_OFFSET + 17);
	//idt_set_gate(IRQ_OFFSET + 18, (uintptr_t)_irq18, 0x08, 0x8E);
	//ioapic_enable_interrupt_pci(ioapic_remap_interrupt(18), IRQ_OFFSET + 18);
	//idt_set_gate(IRQ_OFFSET + 21, (uintptr_t)_irq19, 0x08, 0x8E);
	//ioapic_enable_interrupt_pci(ioapic_remap_interrupt(21), IRQ_OFFSET + 21);



	outb(rtl->BaseAddress + 0x37, 0x0C);


	kprintf("MEdia statudsfsd: 0x%X", inb(rtl->BaseAddress + 0x58));

	//while(true);

	// Free device for now
	//kheap_free(rtl);
}