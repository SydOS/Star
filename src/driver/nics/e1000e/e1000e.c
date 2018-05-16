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
#include <kernel/lock.h>
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
    { 0x105E, "PRO 1000 PT" },

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
    e1000e_write(e1000eDevice, E1000E_REG_MDIC, mdi);
    
    // Wait for PHY.
    uint32_t timeout = 100;
    while (!(e1000e_read(e1000eDevice, E1000E_REG_MDIC) & E1000E_PHY_READY)) {
        if (!timeout) {
            kprintf("E1000E: PHY read timeout!\n");
            break;
        }
        sleep(1);
        timeout--;
    }
    return e1000e_read(e1000eDevice, E1000E_REG_MDIC);
}

static bool e1000e_send_bytes(e1000e_t *e1000eDevice, const void *data, uint16_t length) {
    // For now if a packet is bigger than 4KB, reject.
    if (length > PAGE_SIZE_4K)
        return false;

    // Get index.
    spinlock_lock(&e1000eDevice->TransmitIndexLock);
    uint8_t descIndex = e1000eDevice->CurrentTransmitDesc;
    e1000eDevice->CurrentTransmitDesc = (e1000eDevice->CurrentTransmitDesc + 1) % E1000E_TRANSMIT_DESC_COUNT;

    // Fill descriptor.
    memset(e1000eDevice->TransmitDescs + descIndex, 0, sizeof(e1000e_transmit_desc_t));
    e1000eDevice->TransmitDescs[descIndex].Length = length;
    e1000eDevice->TransmitDescs[descIndex].Command = E1000E_TRANSMIT_CMD_EOP | E1000E_TRANSMIT_CMD_IFCS | E1000E_TRANSMIT_CMD_RS;
    memcpy(e1000eDevice->TransmitBuffers[descIndex], data, length);

    // Send packet.
    kprintf("E1000E: sending!\n");
    e1000eDevice->TransmitDescs[descIndex].Status = 0;
    e1000e_write(e1000eDevice, E1000E_REG_TDT0, e1000eDevice->CurrentTransmitDesc);
    spinlock_release(&e1000eDevice->TransmitIndexLock);

    while (!(e1000eDevice->TransmitDescs[descIndex].Status & 0xFF)) {
        sleep(1000);
        kprintf("status 0x%X\n", e1000eDevice->TransmitDescs[descIndex].Status);
    }
    return true;
}

static void e1000e_get_mac_addr(e1000e_t *e1000eDevice) {
    // Get value of RAL0 and RAH0, which contains the MAC address of the card.
    uint32_t ral = e1000e_read(e1000eDevice, E1000E_REG_RAL);
    uint32_t rah = e1000e_read(e1000eDevice, E1000E_REG_RAH);

    // Decode MAC.
    e1000eDevice->MacAddress[0] = ral & 0xFF;
    e1000eDevice->MacAddress[1] = (ral >> 8) & 0xFF;
    e1000eDevice->MacAddress[2] = (ral >> 16) & 0xFF;
    e1000eDevice->MacAddress[3] = (ral >> 24) & 0xFF;
    e1000eDevice->MacAddress[4] = rah & 0xFF;
    e1000eDevice->MacAddress[5] = (rah >> 8) & 0xFF;
}

void e1000e_reset(e1000e_t *e1000eDevice) {
    // Reset card.
    e1000e_write(e1000eDevice, E1000E_REG_CTRL, e1000e_read(e1000eDevice, E1000E_REG_CTRL) | (1 << 26) | (1 << 31));
    sleep(20);
}

static bool e1000e_callback(pci_device_t *pciDevice) {
    // Get value of interrupt register.
    uint32_t intReg = e1000e_read((e1000e_t*)pciDevice->DriverObject, E1000E_REG_ICR);

    // If no interrupt bits are set, this device wasn't the one that raised the interrupt.
    if (intReg == 0)
        return false;

    kprintf("E1000E: IRQ raised (0x%X)!\n", intReg);
    if (intReg & E1000E_INT_LSC) {
        kprintf("E1000E: Link change!\n");
        kprintf("E1000E: RTCL: 0x%X\n", e1000e_read((e1000e_t*)pciDevice->DriverObject, E1000E_REG_RCTL));

        // Get status register.
        uint32_t status = e1000e_read((e1000e_t*)pciDevice->DriverObject, E1000E_REG_STATUS);
        if (status & E1000E_STATUS_LU) {
            char *speed = "10 Mbps";
            if ((status & E1000E_STATUS_SPEED_1000_2) || (status & E1000E_STATUS_SPEED_1000))
                speed = "1000 Mbps";
            else if (status & E1000E_STATUS_SPEED_100)
                speed = "100 Mbps";
            kprintf("E1000E: Link connected at %s, %s-duplex.\n", speed, status & E1000E_STATUS_FD ? "full" : "half");
        }
        else {
            kprintf("E1000E: Link disconnected.\n");
        }
    }

    // Clear interrupt bits.
    e1000e_write((e1000e_t*)pciDevice->DriverObject, E1000E_REG_ICR, -1);
    return true;
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

    // Create E1000e object.
    e1000e_t *e1000eDevice = (e1000e_t*)kheap_alloc(sizeof(e1000e_t));
    memset(e1000eDevice, 0, sizeof(e1000e_t));
    e1000eDevice->BasePointer = paging_device_alloc(pciDevice->BaseAddresses[0].BaseAddress, pciDevice->BaseAddresses[0].BaseAddress + 0x1F000);
    kprintf("E1000E: Matched %s!\n", e1000eDevices[idIndex].DeviceString);
    
    // Register driver object and IRQ handler with PCI device object.
    pciDevice->DriverObject = e1000eDevice;
    pciDevice->InterruptHandler = e1000e_callback;

    // REset.
   // uint32_t *bdd = (uint32_t*)(e1000eDevice->BasePointer + 0x00);
  //  *bdd |= (1 << 2);
  //  while (*bdd & (1 << 19));
  //  *(uint32_t*)(e1000eDevice->BasePointer + 0xD8) = 0xFFFFFFFF;
  //  *(uint32_t*)(e1000eDevice->BasePointer + 0x100) = 0;
 //   *(uint32_t*)(e1000eDevice->BasePointer + 0x400) = 0x8;
    kprintf("E1000e: Status: 0x%X\n", *(uint32_t*)(e1000eDevice->BasePointer + 0x08));
    sleep(1000);

    //while (*(uint32_t*)(buffer + 0x05B54) & 0x40);
    kprintf("E1000E: Resetting card...\n");
    e1000e_reset(e1000eDevice);

    // Get MAC address.
    e1000e_get_mac_addr(e1000eDevice);
    kprintf("E1000E: MAC address: %2X:%2X:%2X:%2X:%2X:%2X\n", e1000eDevice->MacAddress[0], e1000eDevice->MacAddress[1],
        e1000eDevice->MacAddress[2], e1000eDevice->MacAddress[3], e1000eDevice->MacAddress[4], e1000eDevice->MacAddress[5]);

    kprintf("E1000e: Status: 0x%X\n", *(uint32_t*)(e1000eDevice->BasePointer + 0x08));
    e1000e_write(e1000eDevice, E1000E_REG_IMS, 0xFFFFFFFF);

    // Get page for receive and transmit descriptors.
    e1000eDevice->DescPage = pmm_pop_frame();
    e1000eDevice->DescPtr = paging_device_alloc(e1000eDevice->DescPage, e1000eDevice->DescPage);
    memset(e1000eDevice->DescPtr, 0, PAGE_SIZE_4K);

    // Initialize receive descriptors.
    e1000eDevice->ReceiveDescs = (e1000e_receive_desc_t*)e1000eDevice->DescPtr;
    kprintf("E1000E: Initializing %u receive descriptors at 0x%p...\n", E1000E_RECEIVE_DESC_COUNT, e1000eDevice->ReceiveDescs);
    for (uint8_t rxDesc = 0; rxDesc < E1000E_RECEIVE_DESC_COUNT; rxDesc++)
        e1000eDevice->ReceiveDescs[rxDesc].BufferAddress = pmm_pop_frame();

    // Set location and size of receive descriptor buffer.
    e1000e_write(e1000eDevice, E1000E_REG_RDBAL0, (uint32_t)(e1000eDevice->DescPage & 0xFFFFFFFF));
    e1000e_write(e1000eDevice, E1000E_REG_RDBAH0, (uint32_t)((e1000eDevice->DescPage >> 32) & 0xFFFFFFFF));
    e1000e_write(e1000eDevice, E1000E_REG_RDLEN0, E1000E_RECEIVE_DESC_POOL_SIZE);

    // Set current receive descriptors.
    e1000e_write(e1000eDevice, E1000E_REG_RDH0, 0);
    e1000e_write(e1000eDevice, E1000E_REG_RDT0, E1000E_RECEIVE_DESC_COUNT - 1);

    // Initialize transmit descriptors.
    e1000eDevice->TransmitDescs = (e1000e_transmit_desc_t*)(e1000eDevice->DescPtr + E1000E_RECEIVE_DESC_POOL_SIZE);
    kprintf("E1000E: Initializing %u transmit descriptors at 0x%p...\n", E1000E_TRANSMIT_DESC_COUNT, e1000eDevice->TransmitDescs);
    for (uint8_t txDesc = 0; txDesc < E1000E_TRANSMIT_DESC_COUNT; txDesc++) {
        uint64_t page = pmm_pop_frame();
        e1000eDevice->TransmitDescs[txDesc].BufferAddress = page;
        e1000eDevice->TransmitBuffers[txDesc] = paging_device_alloc(page, page);
    }

    // Set location and size of transmit descriptor buffer.
    e1000e_write(e1000eDevice, E1000E_REG_TDBAL0, (uint32_t)((e1000eDevice->DescPage + E1000E_RECEIVE_DESC_POOL_SIZE) & 0xFFFFFFFF));
    e1000e_write(e1000eDevice, E1000E_REG_TDBAH0, (uint32_t)(((e1000eDevice->DescPage + E1000E_RECEIVE_DESC_POOL_SIZE) >> 32) & 0xFFFFFFFF));
    e1000e_write(e1000eDevice, E1000E_REG_TDLEN0, E1000E_TRANSMIT_DESC_POOL_SIZE);

    // Set current transmit descriptors.
    e1000e_write(e1000eDevice, E1000E_REG_TDH0, 0);
    e1000e_write(e1000eDevice, E1000E_REG_TDT0, 0);

    // Enable the receive function of the card with 4KB buffer descriptors.
    e1000e_write(e1000eDevice, E1000E_REG_RCTL, E1000E_RCTL_EN | E1000E_RCTL_SBP | E1000E_RCTL_UPE | E1000E_RCTL_MPE | E1000E_RCTL_RDMTS_HALF | E1000E_RCTL_BAM | E1000E_RCTL_SECRC | E1000E_RCTL_BSIZE_256 | E1000E_RCTL_BSEX);

    // Enable the transmit function of the card.
  //  e1000e_write(e1000eDevice, E1000E_REG_TCTL, E1000E_TCTL_EN | E1000E_TCTL_PSP | (15 << E1000E_TCTL_CT_SHIFT) | (64 << E1000E_TCTL_COLD_SHIFT) || E1000E_TCTL_RTLC);
    e1000e_write(e1000eDevice, E1000E_REG_TCTL, 0b0110000000000111111000011111010);
    e1000e_write(e1000eDevice, E1000E_REG_TIPG, 0x0060200A);

    kprintf("sleeping for 10 seconds...\n");
    sleep(10000);

    uint8_t data[16];
    data[0] = 0xB4;
    data[1] = 0xB6;
    data[2] = 0x76;
    data[3] = 0x7A;
    data[4] = 0x0E;
    data[5] = 0xD6;
    data[6] = 0x12;
    data[7] = 0x34;
    data[8] = 0x56;
    data[9] = 0x78;
    data[10] = 0x9A;
    data[11] = 0x9A;
    data[12] = 0xFF;
    data[13] = 0xFF;

     kprintf("E1000E: sending!\n");
    e1000e_send_bytes(e1000eDevice, data, 16);
    kprintf("E1000E: packet sent!\n");
    //while(true);
}
