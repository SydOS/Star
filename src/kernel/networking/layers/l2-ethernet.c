/*
 * File: l2-ethernet.c
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
#include <string.h>
#include <kprint.h>

#include <kernel/memory/kheap.h>

#include <kernel/networking/networking.h>
#include <kernel/networking/layers/l2-ethernet.h>

ethernet_frame_t* l2_ethernet_create_frame(uint8_t* MACDest, uint8_t* MACSrc, 
							  uint16_t Ethertype, uint16_t payloadSize, 
							  void* payloadPointer,  uint16_t *FrameSize) {
	// TODO: Check if frame size is under 60 bytes and pad it
	// TODO: Check if payload exceeds 1500 bytes
	// Write the size of our frame to FrameSize pointer
	*FrameSize = sizeof(ethernet_frame_t) + payloadSize;
	if (*FrameSize < 60) {
		*FrameSize = 60;
	}
	kprintf("L2Eth: frame size of 0x%X calculated, (Eth frame 0x%X | payloadSize 0x%X)\n", 
			*FrameSize, sizeof(ethernet_frame_t), payloadSize);

	// Allocate the RAM for our new Ethernet frame to live in
	ethernet_frame_t *frame = (ethernet_frame_t*)kheap_alloc(*FrameSize);
	kprintf("L2Eth: allocated frame size of 0x%X\n", sizeof(ethernet_frame_t));
	// Clear out the frame's memory space
	memset(frame, 0, *FrameSize);
	kprintf("L2Eth: cleared frame with 0s\n");

	// Copy the 6 bytes a MAC address should be
	// This is the destination MAC address
	memcpy(frame->MACDest, MACDest, NET_MAC_LENGTH);
	kprintf("L2Eth: copied destination MAC address to 0x%X\n", &frame->MACDest);
	// This is the source MAC address
	memcpy(frame->MACSrc, MACSrc, NET_MAC_LENGTH);
	kprintf("L2Eth: copied source MAC address to 0x%X\n", &frame->MACSrc);

	// Copy Ethertype
	frame->Ethertype = (Ethertype << 8) | (Ethertype >> 8);
	kprintf("L2Eth: set Ethertype to 0x%X\n", Ethertype);

	// Copy payload data to end of header
	memcpy((void*)frame + sizeof(ethernet_frame_t), payloadPointer, payloadSize);
	kprintf("L2Eth: copied payload to frame\n");

	// Return pointer to our new frame
	return frame;
}