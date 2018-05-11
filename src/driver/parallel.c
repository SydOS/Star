/*
 * File: parallel.c
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
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/parallel.h>

// https://wiki.osdev.org/Parallel_port
// http://retired.beyondlogic.org/spp/parallel.htm
// http://retired.beyondlogic.org/epp/epp.htm

#define LPT1 0x378
#define LPT2 0x278
#define LPT3 0x3BC

void parallel_reset(uint16_t port)
{
	outb(PARALLEL_CONTROL_PORT(port), 0x00);
	sleep(10);
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);
}

void parallel_sendbyte(uint16_t port, unsigned char data)
{
	// Wait for device to be receptive.
	while (!(inb(PARALLEL_STATUS_PORT(port)) & PARALLEL_STATUS_BUSY))
		sleep(10);

	// Write byte to data port.
	outb(port, data);

	// Pulse strobe line to tell end device to read data.
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_STROBE | PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);
	sleep(10);
	outb(PARALLEL_CONTROL_PORT(port), PARALLEL_CONTROL_AUTOLF | PARALLEL_CONTROL_INIT | PARALLEL_CONTROL_SELECT);

	// Wait for end device to finish processing.
	while (!(inb(PARALLEL_STATUS_PORT(port)) & PARALLEL_STATUS_BUSY))
		sleep(10);
}
