/*
 * File: bcm440x.h
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

#ifndef BCM440X_H
#define BCM440X_H

#include <main.h>
#include <driver/pci.h>

#define BCM440X_VENDOR_ID               0x14E4
#define BCM440X_DEVICE_ID_BCM4401_AX    0x4401
#define BCM440X_DEVICE_ID_BCM4402_AX    0x4402
#define BCM440X_DEVICE_ID_BCM4401_B0    0x170C

extern bool bcm440x_init(pci_device_t *pciDevice);

#endif
