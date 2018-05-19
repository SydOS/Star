/*
 * File: bcm440x.c
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
#include <driver/nics/bcm440x.h>

#include <driver/pci.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/interrupts/irqs.h>

#include <kernel/memory/paging.h>

bool bcm440x_init(pci_device_t *pciDevice) {
    // Is the PCI device a BCM440x?
    if (!(pciDevice->Class == PCI_CLASS_NETWORK && pciDevice->Subclass == PCI_SUBCLASS_NETWORK_ETHERNET
        && pciDevice->VendorId == BCM440X_VENDOR_ID
        && (pciDevice->DeviceId == BCM440X_DEVICE_ID_BCM4401_AX || pciDevice->DeviceId == BCM440X_DEVICE_ID_BCM4402_AX || pciDevice->DeviceId == BCM440X_DEVICE_ID_BCM4401_B0)))
        return false;

    kprintf("BCM440X: Matched!\n");
    //kprintf("BCM440X: SPROM reg: 0x%X\n", pci_config_read_dword(pciDevice, 0x88));

   /* kprintf("current bar0: 0x%X\n", pci_config_read_dword(pciDevice, 0x80));
    pci_config_write_dword(pciDevice, 0x80, 0x18000000);

    uintptr_t buffer = (uintptr_t)paging_device_alloc(pciDevice->BaseAddresses[0].BaseAddress, pciDevice->BaseAddresses[0].BaseAddress);

    *(volatile uint32_t*)(buffer + 0xF98) = 0x30001;
    sleep(1);
    *(volatile uint32_t*)(buffer + 0xF98) = 0x30000;
    sleep(1);
    *(volatile uint32_t*)(buffer + 0xF98) = 0x10000;
    sleep(1);
    *(volatile uint32_t*)(buffer + 0x410) = 0x81;
    sleep(1);
    kprintf("BCM440X: MII status 0x%X\n", *(volatile uint32_t*)(buffer + 0x410));
    uint32_t *miiData = (uint32_t*)(buffer + 0x414);
    *miiData = 0x60060000;
    sleep(1);
    kprintf("mii 0x%X\n", *miiData);

    kprintf("mapped buffer\n");
    uint32_t value = *(volatile uint32_t*)(buffer + 0x00);
    kprintf("mapped buffer22\n");
    kprintf("BCM440X: MAC1 0x%X\n", value);
    //kprintf("BCM440X: MAC2 0x%X\n", *(volatile uint32_t*)(buffer + 0x8C));
    */
    return true;
}
