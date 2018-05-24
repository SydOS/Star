/*
 * File: l3-ipv4.c
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

#include <byteswap.h>

#include <kernel/memory/kheap.h>

#include <kernel/networking/layers/l3-ipv4.h>

ipv4_frame_t* l3_ipv4_create_frame(uint8_t* SourceIP, uint8_t* DestinationIP, 
                                   uint8_t protocol, uint16_t payloadSize, 
                                   void* payloadPointer, uint16_t *FrameSize) {

    *FrameSize = sizeof(ipv4_frame_t) + payloadSize;
	kprintf("L3IPv4: frame size of 0x%X calculated, (IPv4 frame 0x%X | payloadSize 0x%X)\n", *FrameSize, sizeof(ipv4_frame_t), payloadSize);
    
    // Allocate the RAM for our new IPv4 frame to live in
	ipv4_frame_t *frame = (ipv4_frame_t*)kheap_alloc(*FrameSize);
	kprintf("L3IPv4: allocated frame size of 0x%X\n", sizeof(ipv4_frame_t));
	// Clear out the frame's memory space
	memset(frame, 0, *FrameSize);
	kprintf("L3IPv4: cleared frame with 0s\n");

    // Set generic values
    // TODO: Flip first byte
    // TODO: First byte of TotalLength or the DSCP/ECN byte doesn't seem to be there
    // TODO: Generate checksum
    frame->Version = 4;
    frame->IHL = 5;
    frame->DSCP = 0;
    frame->ECN = 0;
    frame->TotalLength = FrameSize;
    frame->Identification = 1;
    frame->Flags = 0;
    frame->FragmentationOffset = 0;
    frame->TimeToLive = 64;
    frame->Protocol = protocol;
    kprintf("L3IPv4: mass copied generic frame items\n");

    // Copy the 4 bytes an IP address should be
	// This is the destination MAC address
	memcpy(frame->SourceIP, SourceIP, 4);
	kprintf("L3IPv4: copied destination IP address to 0x%X\n", &frame->SourceIP);
	// This is the source MAC address
	memcpy(frame->DestinationIP, DestinationIP, 4);
	kprintf("L3IPv4: copied source IP address to 0x%X\n", &frame->DestinationIP);

    memcpy((void*)frame + sizeof(ipv4_frame_t), payloadPointer, payloadSize);
	kprintf("L3IPv4: copied payload to frame\n");

    // Return pointer to our new frame
	return frame;
}