/*
 * File: l2-ethernet.h
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

#ifndef NETWORKING_ETHERNET_H
#define NETWORKING_ETHERNET_H

#include <main.h>
#include <kernel/networking/networking.h>

typedef struct {
	uint8_t MACDest[6];
	uint8_t MACSrc[6];
	uint16_t Ethertype;
	// The payload comes after, but can be variable length, so just allocate
	// the memory size to be bigger than this actual struct to fit it.
} __attribute__((packed)) ethernet_frame_t;

extern ethernet_frame_t* l2_ethernet_create_frame(uint8_t* MACDest, uint8_t* MACSrc, 
							  uint16_t Ethertype, uint16_t payloadSize, 
							  void* payloadPointer,  uint16_t *FrameSize);

extern ethernet_frame_t* l2_ethernet_handle_packet(net_packet_t *packet);

#endif
