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
#include <string.h>
#include <driver/nics/rtl8139.h>

#include <driver/pci.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/interrupts/irqs.h>

#include <kernel/memory/paging.h>

static inline void rtl8139_writeb(rtl8139_t *rtlDevice, uint16_t reg, uint8_t value) {
    outb(rtlDevice->BaseAddress + reg, value);
}

static inline void rtl8139_writew(rtl8139_t *rtlDevice, uint16_t reg, uint16_t value) {
    outw(rtlDevice->BaseAddress + reg, value);
}

static inline void rtl8139_writel(rtl8139_t *rtlDevice, uint16_t reg, uint32_t value) {
    outl(rtlDevice->BaseAddress + reg, value);
}

static inline uint8_t rtl8139_readb(rtl8139_t *rtlDevice, uint16_t reg) {
    return inb(rtlDevice->BaseAddress + reg);
}

static inline uint16_t rtl8139_readw(rtl8139_t *rtlDevice, uint16_t reg) {
    return inw(rtlDevice->BaseAddress + reg);
}

static inline uint32_t rtl8139_readl(rtl8139_t *rtlDevice, uint16_t reg) {
    return inl(rtlDevice->BaseAddress + reg);
}

bool rtl8139_send_bytes(rtl8139_t *rtlDevice, const void *data, uint32_t length) {
    // Ensure length is under 2KB.
    if (length > RTL8139_TX_BUFFER_SIZE)
        return false;

    // Determine value of TX status register.
    uint32_t txStatus = (length & 0x1FFF);

    // Determine TX status register to use. Card uses all 4 in a round robin fashion.
    switch (rtlDevice->CurrentTxBuffer) {
        case 0:
            // Copy data to TX buffer 0 and send.
            memcpy(rtlDevice->TxBuffer0, data, length);
            rtl8139_writel(rtlDevice, RTL8139_REG_TX_STATUS0, txStatus);
            break;

        case 1:
            // Copy data to TX buffer 1 and send.
            memcpy(rtlDevice->TxBuffer1, data, length);
            rtl8139_writel(rtlDevice, RTL8139_REG_TX_STATUS1, txStatus);
            break;

        case 2:
            // Copy data to TX buffer 2 and send.
            memcpy(rtlDevice->TxBuffer2, data, length);
            rtl8139_writel(rtlDevice, RTL8139_REG_TX_STATUS2, txStatus);
            break;

        case 3:
            // Copy data to TX buffer 3 and send.
            memcpy(rtlDevice->TxBuffer3, data, length);
            rtl8139_writel(rtlDevice, RTL8139_REG_TX_STATUS3, txStatus);
            break;
    }

    // Move to next TX buffer, or back to start if we hit 4.
    rtlDevice->CurrentTxBuffer++;
    rtlDevice->CurrentTxBuffer %= RTL8139_TX_BUFFER_COUNT;
    return true;
}

static bool rtl8139_callback(pci_device_t *dev) {
    // If no interrupts were raised by the card, don't handle it.
    rtl8139_t *rtlDevice = (rtl8139_t*)dev->DriverObject;
    if (inw(rtlDevice->BaseAddress + 0x3E) == 0)
        return false;

    kprintf("RTL8139: Current interrupt bits: 0x%X\n", inw(rtlDevice->BaseAddress + 0x3E));
    outw(rtlDevice->BaseAddress + 0x3E, 0xFFFF);
    kprintf("RTL8139: Cleared interrupt\n");
    return true;
}

bool rtl8139_init(pci_device_t *pciDevice) {
    // Is this actually a NIC?
    if (!(pciDevice->Class == 0x02 && pciDevice->Subclass == 0)) {
        return false;
    }

    // Is the PCI device an RTL8139?
    if (!(pciDevice->VendorId == 0x10EC && pciDevice->DeviceId == 0x8139)) {
        return false;
    }

    // Allocate RTL8139 struct.
    rtl8139_t *rtlDevice = (rtl8139_t*)kheap_alloc(sizeof(rtl8139_t));
    memset(rtlDevice, 0, sizeof(rtl8139_t));
    rtlDevice->PciDevice = pciDevice;
    pciDevice->DriverObject = rtlDevice;
    kprintf("\e[35mRTL8139: Pointed RTL struct to DriverObject\n");
    pciDevice->InterruptHandler = rtl8139_callback;
    kprintf("RTL8139: Pointed RTL callback handler function to InterruptHandler\n");

    // Setup base address
    rtlDevice->BaseAddress = pciDevice->BaseAddresses[0].BaseAddress;

    // Check base address is valid
    if(rtlDevice->BaseAddress == 0) {
        kprintf("RTL8139: INVALID BAR\n");
        kheap_free(rtlDevice);
        return false;
    }
    kprintf("RTL8139: using BAR 0x%X\n", rtlDevice->BaseAddress);

    // Enable PCI busmastering.
    pci_enable_busmaster(pciDevice);
    kprintf("RTL8139: Enabled PCI busmastering\n");

    // Bring card out of low power mode.
    rtl8139_writeb(rtlDevice, RTL8139_REG_CONFIG1, 0x00);
    kprintf("RTL8139: Brought card out of low power mode.\n");

    // Read MAC address from RTL8139.
    for (uint8_t i = 0; i < 6; i++)
        rtlDevice->MacAddress[i] = rtl8139_readb(rtlDevice, RTL8139_REG_IDR0 + i);
    kprintf("RTL8139: MAC %X:%X:%X:%X:%X:%X\n", rtlDevice->MacAddress[0], rtlDevice->MacAddress[1],
        rtlDevice->MacAddress[2], rtlDevice->MacAddress[3], rtlDevice->MacAddress[4],
        rtlDevice->MacAddress[5]);

    // Reset card by setting reset bit and waiting for bit to clear.
    rtl8139_writeb(rtlDevice, RTL8139_REG_CMD, RTL8139_CMD_RESET);
    while (rtl8139_readb(rtlDevice, RTL8139_REG_CMD) & RTL8139_CMD_RESET);
    kprintf("RTL8139: Card reset!\n");	

    // Get a 64KB DMA frame to use for buffers.
    if (!pmm_dma_get_free_frame(&rtlDevice->DmaFrame))
        panic("RTL8139: Unable to get DMA frame!\n");
    memset((void*)rtlDevice->DmaFrame, 0, PAGE_SIZE_64K);
    rtlDevice->RxBuffer = (uint8_t*)rtlDevice->DmaFrame;
    rtlDevice->TxBuffer0 = (uint8_t*)((uintptr_t)rtlDevice->RxBuffer + RTL8139_RX_BUFFER_SIZE);
    rtlDevice->TxBuffer1 = (uint8_t*)((uintptr_t)rtlDevice->TxBuffer0 + RTL8139_TX_BUFFER_SIZE);
    rtlDevice->TxBuffer2 = (uint8_t*)((uintptr_t)rtlDevice->TxBuffer1 + RTL8139_TX_BUFFER_SIZE);
    rtlDevice->TxBuffer3 = (uint8_t*)((uintptr_t)rtlDevice->TxBuffer2 + RTL8139_TX_BUFFER_SIZE);
    kprintf("RTL8139: Allocated DMA space for RX and TX buffers.\n");

    // Set IMR + ISR
    outw(rtlDevice->BaseAddress + 0x3C, 0xFFFF);
    kprintf("RTL8139: Set IMR + ISR\n");

    // Enable RX and TX.
    rtl8139_writeb(rtlDevice, RTL8139_REG_CMD, RTL8139_CMD_TX_ENABLE | RTL8139_CMD_RX_ENABLE);
    kprintf("RTL8139: Enabled RX and TX!\n");

    // https://forum.osdev.org/viewtopic.php?t=25750&p=214461.
    // Set RX buffer physical memory location.
    rtl8139_writel(rtlDevice, RTL8139_REG_RX_BUFFER, (uint32_t)pmm_dma_get_phys((uintptr_t)rtlDevice->RxBuffer));
    kprintf("RTL8139: Transmitted RX buffer location to card.\n");

    // Configure RX buffer.
    rtl8139_writel(rtlDevice, RTL8139_REG_RCR, RTL8139_RCR_ACCEPT_ALL_PACKETS | RTL8139_RCR_ACCPET_PHYS_MATCH
        | RTL8139_RCR_ACCEPT_MULTICAST | RTL8139_RCR_ACCEPT_RUNT | RTL8139_RCR_ACCEPT_ERROR | RTL8139_RCR_WRAP | RTL8139_RCR_BUFFER_LENGTH_32K);
    kprintf("RTL8139: Configured RX buffer.\n");

    // Configure TX buffers.
    rtl8139_writel(rtlDevice, RTL8139_REG_TX_BUFFER0, (uint32_t)pmm_dma_get_phys((uintptr_t)rtlDevice->TxBuffer0));
    rtl8139_writel(rtlDevice, RTL8139_REG_TX_BUFFER1, (uint32_t)pmm_dma_get_phys((uintptr_t)rtlDevice->TxBuffer1));
    rtl8139_writel(rtlDevice, RTL8139_REG_TX_BUFFER2, (uint32_t)pmm_dma_get_phys((uintptr_t)rtlDevice->TxBuffer2));
    rtl8139_writel(rtlDevice, RTL8139_REG_TX_BUFFER3, (uint32_t)pmm_dma_get_phys((uintptr_t)rtlDevice->TxBuffer3));
    kprintf("RTL8139: Configured TX buffers.\n");

    // Just send some garbage to prove it works in Wireshark.
    uint8_t ddd[54];
    rtl8139_send_bytes(rtlDevice, ddd, 54);

    // Ask for media status of RTL8139
    kprintf("RTL8139: Media status: 0x%X\n", inb(rtlDevice->BaseAddress + 0x58));
    kprintf("RTL8139: Mode status: 0x%X\n", inw(rtlDevice->BaseAddress + 0x64));
    kprintf("RTL8139: CR: 0x%X\n", inb(rtlDevice->BaseAddress + 0x37));

    // Return true, we have handled the PCI device passed to us
    return true;
}