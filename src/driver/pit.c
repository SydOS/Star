/*
 * File: pit.c
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
#include <driver/pit.h>

// Starts a counter on the PIT.
void pit_startcounter(uint32_t freq, uint8_t counter, uint8_t mode) {
	// Determine divisor.
	uint16_t divisor = PIT_BASE_FREQ / freq;

	// Send command to PIT.
	uint8_t command = 0;
	command = (command & ~PIT_CMD_MASK_BINCOUNT) | PIT_CMD_BINCOUNT_BINARY;
	command = (command & ~PIT_CMD_MASK_MODE) | mode;
	command = (command & ~PIT_CMD_MASK_ACCESS) | PIT_CMD_ACCESS_DATA;
	command = (command & ~PIT_CMD_MASK_COUNTER) | counter;
	outb(PIT_PORT_COMMAND, command);

	// Send divisor.
	switch (counter) {
		case PIT_CMD_COUNTER0:
			outb(PIT_PORT_CHANNEL0, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL0, (divisor >> 8) & 0xFF);
			break;

		case PIT_CMD_COUNTER1:
			outb(PIT_PORT_CHANNEL1, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL1, (divisor >> 8) & 0xFF);
			break;

		case PIT_CMD_COUNTER2:
			outb(PIT_PORT_CHANNEL2, divisor & 0xFF);
			outb(PIT_PORT_CHANNEL2, (divisor >> 8) & 0xFF);
			break;
	}
}

// Initialize the PIT.
void pit_init(void) {
	// Start main timer at 1 tick = 1 ms.
	kprintf("PIT: Initializing...\n");
	pit_startcounter(1000, PIT_CMD_COUNTER0, PIT_CMD_MODE_SQUAREWAVEGEN);
	kprintf("PIT: Initialized!\n");
}
