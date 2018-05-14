/*
 * File: e1000e.c
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
#include <driver/nics/e1000e.h>

#include <driver/pci.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/interrupts/irqs.h>

#include <kernel/memory/paging.h>

static const uint16_t e1000eVendorIds[] = {
    0x1501,

    0x10E5,

    0x10F5,
    0x10BF,
    0x10CB,

    0x10CC,
    0x10CD,
    0x10CE,
    
    0x10DE,
    0x10DF,
    0xFFFF
};

static const char* e1000eProductStrings[] = {
    "82567V-3",

    "82567LM-4",

    "82567LM",
    "82567LF",
    "82567V",

    "82567LM-2",
    "82567LF-2",
    "82567V-2",

    "82567LM-3",
    "82567LF-3",
    ""
};

bool e1000e_init(pci_device_t *pciDevice) {
    // Is the PCI device an Intel networking device?
    if (!(pciDevice->Class == PCI_CLASS_NETWORK && pciDevice->Subclass == PCI_SUBCLASS_NETWORK_ETHERNET
        && pciDevice->VendorId == E1000E_VENDOR_ID))
        return false;

    // Does the device match any IDs?
    uint32_t idIndex = 0;
    while (pciDevice->DeviceId != e1000eVendorIds[idIndex] && e1000eVendorIds[idIndex] != 0xFFFF)
        idIndex++;
    if (e1000eVendorIds[idIndex] == 0xFFFF)
        return false;

    void *buffer = paging_device_alloc(pciDevice->BaseAddresses[0].BaseAddress, pciDevice->BaseAddresses[0].BaseAddress);

    

    kprintf("E1000E: Matched %s!\n", e1000eProductStrings[idIndex]);
    kprintf("E1000e: Status: 0x%X\n", *(uint32_t*)(buffer + 0x08));

    while(true);
}
