/*
 * File: parallel.h
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

// Ports.
#define PARALLEL_DATA_PORT(p)           (p+0)
#define PARALLEL_STATUS_PORT(p)         (p+1)
#define PARALLEL_CONTROL_PORT(p)        (p+2)
#define PARALLEL_EPP_ADDR_PORT(p)       (p+3)
#define PARALLEL_EPP_DATA_PORT(p)       (p+4)
#define PARALLEL_ECP_DATA_PORT(p)       (p+0x400)
#define PARALLEL_ECP_REGB_PORT(p)       (p+0x401)
#define PARALLEL_ECP_CONTROL_PORT(p)    (p+0x402)

// Parallel SPP status register bits.
#define PARALLEL_STATUS_IRQ         0x04
#define PARALLEL_STATUS_ERROR       0x08
#define PARALLEL_STATUS_SELECT_IN   0x10
#define PARALLEL_STATUS_PAPER_OUT   0x20
#define PARALLEL_STATUS_ACK         0x40
#define PARALLEL_STATUS_BUSY        0x80

// Parallel SPP control register bits.
#define PARALLEL_CONTROL_STROBE     0x01
#define PARALLEL_CONTROL_AUTOLF     0x02
#define PARALLEL_CONTROL_INIT       0x04
#define PARALLEL_CONTROL_SELECT     0x08
#define PARALLEL_CONTROL_IRQACK     0x10
#define PARALLEL_CONTROL_BIDI       0x20


extern void parallel_sendbyte(uint16_t port, unsigned char data);