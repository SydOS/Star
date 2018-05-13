/*
 * File: rtl8139.h
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

#ifndef RTL8139_H
#define RTL8139_H

#include <main.h>
#include <driver/pci.h>

// Registers
#define RTL8139_REG_IDR0    0x00
#define RTL8139_REG_IDR1    0x01
#define RTL8139_REG_IDR2    0x02
#define RTL8139_REG_IDR3    0x03
#define RTL8139_REG_IDR4    0x04
#define RTL8139_REG_IDR5    0x05

#define RTL8139_REG_TX_STATUS0  0x10
#define RTL8139_REG_TX_STATUS1  0x14
#define RTL8139_REG_TX_STATUS2  0x18
#define RTL8139_REG_TX_STATUS3  0x1C
#define RTL8139_REG_TX_BUFFER0  0x20
#define RTL8139_REG_TX_BUFFER1  0x24
#define RTL8139_REG_TX_BUFFER2  0x28
#define RTL8139_REG_TX_BUFFER3  0x2C
#define RTL8139_REG_RX_BUFFER   0x30

#define RTL8139_REG_CMD     0x37
#define RTL8139_REG_TCR     0x40
#define RTL8139_REG_RCR     0x44
#define RTL8139_REG_CONFIG1 0x52

#define RTL8139_CMD_BUFFER_EMPTY    0x01
#define RTL8139_CMD_TX_ENABLE       0x04
#define RTL8139_CMD_RX_ENABLE       0x08
#define RTL8139_CMD_RESET           0x10

#define RTL8139_RCR_BUFFER_LENGTH_8K     0x0
#define RTL8139_RCR_BUFFER_LENGTH_16K    0x1
#define RTL8139_RCR_BUFFER_LENGTH_32K    0x2
#define RTL8139_RCR_BUFFER_LENGTH_64K    0x3

#define RTL8139_RCR_ACCEPT_ALL_PACKETS  0x01
#define RTL8139_RCR_ACCPET_PHYS_MATCH   0x02
#define RTL8139_RCR_ACCEPT_MULTICAST    0x04
#define RTL8139_RCR_ACCEPT_RUNT         0x10
#define RTL8139_RCR_ACCEPT_ERROR        0x20
#define RTL8139_RCR_WRAP                0x80

// RX buffer is 32KB + 16 bytes + 1500 bytes.
#define RTL8139_RX_BUFFER_SIZE      (0x8000 + 16 + 1500)

// TX buffers are 2048 bytes each.
#define RTL8139_TX_BUFFER_SIZE      0x800
#define RTL8139_TX_BUFFER_COUNT     4

typedef struct {
    pci_device_t *PciDevice;
    uint32_t BaseAddress;
    uint8_t MacAddress[6];

    uintptr_t DmaFrame;
    uint8_t *RxBuffer;
    uint8_t *TxBuffer0;
    uint8_t *TxBuffer1;
    uint8_t *TxBuffer2;
    uint8_t *TxBuffer3;

    uint8_t CurrentTxBuffer;

    bool UsesEeprom;  
} rtl8139_t;

extern bool rtl8139_send_bytes(rtl8139_t *rtlDevice, const void *data, uint32_t length);
extern bool rtl8139_init(pci_device_t *pciDevice);

#endif
