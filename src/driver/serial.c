/*
 * File: serial.c
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
#include <string.h>
#include <driver/serial.h>

#define PORT 0x3f8   /* COM1 */

static bool serialPresent = true;

// http://retired.beyondlogic.org/serial/serial.htm

bool serial_present() {
    return serialPresent;
}

void serial_init() {
    outb(PORT + 1, 0x00);    // Disable all interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00);    //                  (hi byte)
    outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    

    // Place into loopback mode and test.
    outb(SERIAL_REG_MCR(PORT), SERIAL_MCR_LOOPBACK_MODE);
    const char *testString = "SydOS!";
    serial_writes(testString);
    
    for (uint8_t i = 0; i < strlen(testString); i++) {
        // Get char from serial port.
        char c = serial_read();

        // Ensure it matches.
        if (testString[i] != c) {
            serialPresent = false;
            return;
        }
    }

    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    serial_writes("If you're reading this, serial works.\n");
}

int is_transmit_empty() {
   return inb(SERIAL_REG_LSR(PORT)) & SERIAL_LSR_EMPTY_TRANS_HOLDING;
}
 
void serial_write(char a) {
  while (is_transmit_empty() == 0) { };
 
   outb(PORT,a);

   // If newline, print '\r' too.
   if (a == '\n')
    outb(PORT,'\r');
}

void serial_writes(const char* data) {
    for (size_t i = 0; i < strlen(data); i++) {
    serial_write(data[i]);
	}
}

int serial_received() {
    if (!serialPresent)
        return 0;

   return inb(SERIAL_REG_LSR(PORT)) & SERIAL_LSR_DATA_READY;
}
 
char serial_read() {
   while (serial_received() == 0);
 
   return inb(PORT);
}