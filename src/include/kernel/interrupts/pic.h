/*
 * File: pic.h
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

#ifndef PIC_H
#define PIC_H

#include <main.h>

// PIC master and slave ports.
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC2_CMD        0xA0
#define PIC2_DATA       0xA1

// PIC commands.
#define PIC_CMD_8086    0x01
#define PIC_CMD_IRR     0x0A
#define PIC_CMD_ISR     0x0B
#define PIC_CMD_INIT    0x11
#define PIC_CMD_EOI     0x20
#define PIC_CMD_DISABLE 0xFF

extern void pic_enable(void);
extern void pic_disable(void);

extern void pic_eoi(uint32_t irq);
extern uint16_t pic_get_irr(void);
extern uint16_t pic_get_isr(void);
extern uint8_t pic_get_irq(void);
extern void pic_init(void);

#endif
