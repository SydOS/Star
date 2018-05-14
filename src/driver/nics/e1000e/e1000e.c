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

typedef struct {
    uint16_t DeviceId;
    char *DeviceString;
} e1000e_product_id_t;

static const e1000e_product_id_t e1000eDevices[] = {
    // Intel 82566 PHYs.
    { 0x10BD, "82566DM" },
    { 0x294C, "82566DC" },

    // Intel 82567 PHYs.
    { 0x1501, "82567V-3" },
    { 0x10E5, "82567LM-4" },
    { 0x10F5, "82567LM" },
    { 0x10BF, "82567LF" },
    { 0x10CB, "82567V" },
    { 0x10CC, "82567LM-2" },
    { 0x10CD, "82567LF-2" },
    { 0x10CE, "82567V-2" },
    { 0x10DE, "82567LM-3" },
    { 0x10DF, "82567LF-3" },

    // Intel 82574 PHY.
    { 0x10D3, "82574" },

    // Intel 82577 PHYs.
    { 0x10EA, "82577" },
    { 0x10EB, "82577" },

    // Intel 82579 PHYs.
    { 0x1502, "82579LM" },
    { 0x1503, "82579V" },

    // Intel i217 PHYs.
    { 0x153A, "I217-LM" },
    { 0x153B, "I217-V" },

    // Null.
    { 0xFFFF, "" }
};

static inline uint32_t e1000e_read(e1000e_t *e1000eDevice, uint16_t reg) {
    return *(volatile uint32_t*)(e1000eDevice->BasePointer + reg);
}

static inline void e1000e_write(e1000e_t *e1000eDevice, uint16_t reg, uint32_t value) {
    *(volatile uint32_t*)(e1000eDevice->BasePointer + reg) = value;
}


static inline uint32_t e1000e_phy_read(e1000e_t *e1000eDevice, uint16_t reg) {
    uint32_t mdi = (2 << 26) | (2 << 21) | (reg << 16);
    e1000e_write(e1000eDevice, 0x20, mdi);
    sleep(10);
    return e1000e_read(e1000eDevice, 0x20);
}

bool e1000e_init(pci_device_t *pciDevice) {
    // Is the PCI device an Intel networking device?
    if (!(pciDevice->Class == PCI_CLASS_NETWORK && pciDevice->Subclass == PCI_SUBCLASS_NETWORK_ETHERNET
        && pciDevice->VendorId == E1000E_VENDOR_ID))
        return false;

    // Does the device match any IDs?
    uint32_t idIndex = 0;
    while (pciDevice->DeviceId != e1000eDevices[idIndex].DeviceId && e1000eDevices[idIndex].DeviceId != 0xFFFF)
        idIndex++;
    if (e1000eDevices[idIndex].DeviceId == 0xFFFF)
        return false;

    e1000e_t *e1000eDevice = (e1000e_t*)kheap_alloc(sizeof(e1000e_t));

    e1000eDevice->BasePointer = paging_device_alloc(pciDevice->BaseAddresses[0].BaseAddress, pciDevice->BaseAddresses[0].BaseAddress + 0x1F000);

    

    kprintf("E1000E: Matched %s!\n", e1000eDevices[idIndex].DeviceString);
    kprintf("El000e: pci cmd 0x%X\n", pci_config_read_word(pciDevice, PCI_REG_COMMAND));

    // REset.
    uint32_t *bdd = (uint32_t*)(e1000eDevice->BasePointer + 0x00);
    *bdd |= (1 << 2);
    while (*bdd & (1 << 19));
    *(uint32_t*)(e1000eDevice->BasePointer + 0xD8) = 0xFFFFFFFF;
    *(uint32_t*)(e1000eDevice->BasePointer + 0x100) = 0;
    *(uint32_t*)(e1000eDevice->BasePointer + 0x400) = 0x8;
    kprintf("E1000e: Status: 0x%X\n", *(uint32_t*)(e1000eDevice->BasePointer + 0x08));
    sleep(1000);

    //while (*(uint32_t*)(buffer + 0x05B54) & 0x40);
    kprintf("reseting...\n");

   // uint32_t rese = *bdd | (1 << 26) | (1 << 31);
    // Reset card.
    e1000e_write(e1000eDevice, 0x00, e1000e_read(e1000eDevice, 0x00) | (1 << 26) | (1 << 31));
  //  *bdd = rese;
    sleep(20);

    kprintf("E1000e: Status: 0x%X\n", *(uint32_t*)(e1000eDevice->BasePointer + 0x08));
    kprintf("e1000e control 0x%X\n", *bdd);
    *(uint32_t*)(e1000eDevice->BasePointer + 0xD8) = 0xFFFFFFFF;

  //  *(uint32_t*)(e1000eDevice->BasePointer + 0x20) = 0x8210000;
   // sleep(1);
   // kprintf("e1000e: mac 0x%X\n", *(uint32_t*)(e1000eDevice->BasePointer + 0x20));
   //int test = 1;
 // while(test);
   kprintf("E1000E: PHY status 0x%X\n", e1000e_phy_read(e1000eDevice, 0x1));

    kprintf("E1000E: PHY ID1: 0x%X\n", e1000e_phy_read(e1000eDevice, 0x2) & 0xFFFF);
    kprintf("E1000E: PHY ID2: 0x%X\n", e1000e_phy_read(e1000eDevice, 0x3) & 0xFFFF);

    while(true);
}
