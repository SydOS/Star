/*
 * File: arp.c
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
#include <byteswap.h>

#include <kernel/memory/kheap.h>
#include <kernel/networking/layers/l2-ethernet.h>
#include <kernel/networking/protocols/arp.h>
#include <kernel/networking/networking.h>

arp_frame_t* arp_request(uint8_t* SenderMAC, uint8_t* TargetIP) {
	// Allocate memory for frame
	arp_frame_t *frame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));

	// Clear frame with 0s
	memset(frame, 0, sizeof(arp_frame_t));

	// Fill out ARP frame details
	frame->HardwareType = swap_uint16(1);
	frame->ProtocolType = swap_uint16(0x0800);
	frame->HardwareSize = 6;
	frame->ProtocolSize = 4;
	frame->Opcode = swap_uint16(1);
	memcpy((frame->SenderMAC), SenderMAC, NET_MAC_LENGTH);
	for (int x = 0; x < NET_IPV4_LENGTH; x++)
		frame->SenderIP[x] = 0x00;
	for (int x = 0; x < NET_MAC_LENGTH; x++)
		frame->TargetMAC[x] = 0x00;
	memcpy((frame->TargetIP), TargetIP, 4);

	// Return the frame to caller
	return frame;
}

ethernet_frame_t* arp_create_packet(net_device_t* netDevice, uint8_t* targetIP, uint16_t *frameSize) {
	// Generate global broadcast MAC
	uint8_t destMAC[8];
	for (int x = 0; x < NET_MAC_LENGTH; x++) {
		destMAC[x] = 0xFF;
	}

	// Create an ARP request frame
	arp_frame_t *arpFrame = arp_request(netDevice->MacAddress, targetIP);
	// Wrap our ARP request frame in an Ethernet frame
	ethernet_frame_t* frame = l2_ethernet_create_frame(destMAC, netDevice->MacAddress, 0x0806, sizeof(arp_frame_t), arpFrame, frameSize);
	// Free the arpFrame field since we don't need it, it's copied into the Ethernet frame
	kheap_free(arpFrame);
	// Return our new fully packed ARP request packet
	return frame;
}

arp_frame_t* responseFrames[50];

void arp_process_response(ethernet_frame_t* ethFrame) {
	// Generate temporary frame
	arp_frame_t* inFrame = (arp_frame_t*)((uint8_t*)ethFrame+sizeof(ethernet_frame_t));

	// Check if we are waiting for an ARP reply and it is a reply
	if (swap_uint16(inFrame->Opcode) == 2) { // TODO replace with #define for opcode.
		// Free oldest response in the buffer
		kheap_free(responseFrames[50]);
		// Move all current responses up the buffer
		// EG 49 to 50, 48 to 49, 47 to 48
		for (int i = 49; i > 0; i--) {
			responseFrames[i+1] = responseFrames[i];
		}
		// Allocate our newest response frame some memory
		responseFrames[0] = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
		// Copy the frame we got into this space
		memcpy(responseFrames[0], (arp_frame_t*)((uint8_t*)ethFrame+sizeof(ethernet_frame_t)), sizeof(arp_frame_t));
	}
}

arp_frame_t* arp_get_mac_address(net_device_t* netDevice, uint8_t* targetIP) {
	// Check our table to see if a response for the IP is already in it
	// Sweep up the responses, from 0 -> 50
	for (int i = 0; i < 50; i++) {
		// If the responseFrame at index i has the same IP...
		if (memcmp(responseFrames[i]->SenderIP, targetIP, sizeof (targetIP))) {

			// Copy the frame from our buffer to a local variable
			arp_frame_t* responseFrame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
			memcpy(responseFrame, responseFrames[i], sizeof(arp_frame_t));

			// Return with the response
			return responseFrame;
		}
	}

	// If our frames haven't been properly cleared and made yet
	if (responseFrames[0] == 0x0) {
		kprintf("ARP: Generating blank frame\n");
		for (int i = 0; i < 50; i++) {
			// Allocate memory for frame
			responseFrames[i] = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
			// Clear frame with 0s
			memset(responseFrames[i], 0, sizeof(arp_frame_t));
		}
	}

	// Generate global broadcast MAC
	uint16_t frameSize;
	uint8_t destMAC[NET_MAC_LENGTH];
	for (int x = 0; x < NET_MAC_LENGTH; x++) {
		destMAC[x] = 0xFF;
	}

	// Generate and send ARP request
	arp_frame_t *arpFrame = arp_request(netDevice->MacAddress, targetIP);
	ethernet_frame_t* frame = l2_ethernet_create_frame(destMAC, netDevice->MacAddress, 0x0806, sizeof(arp_frame_t), arpFrame, &frameSize);
	netDevice->Send(netDevice, frame, frameSize);
	kheap_free(arpFrame);

	// Wait for a reply
	uint64_t targetTick = timer_ticks() + 2000;
	bool didFindResponse = false;
	arp_frame_t* responseFrame;
	// Wait until we get a response
	while (!didFindResponse) {
		// Sweep up the responses, from 0 -> 50
		for (int i = 0; i < 50; i++) {
			// If the responseFrame at index i has the same IP...
			if (memcmp(responseFrames[i]->SenderIP, targetIP, sizeof (targetIP))) {

				// Copy the frame from our buffer to a local variable
				responseFrame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
				memcpy(responseFrame, responseFrames[i], sizeof(arp_frame_t));

				// We got our response
				didFindResponse = true;
			}
		}

		// If the timer ran equal or over to our timeout length
		if (timer_ticks() >= targetTick) {
			kprintf("ARP: request timeout\n");
			// Allocate memory for frame
			responseFrame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));

			// Clear frame with 0s and return
			memset(responseFrame, 0, sizeof(arp_frame_t));
			return responseFrame;
		}
	}
	
	// Print out ARP response info
	kprintf("ARP: IP for %2X:%2X:%2X:%2X:%2X:%2X is %u.%u.%u.%u\n",
			responseFrame->SenderMAC[0], responseFrame->SenderMAC[1], responseFrame->SenderMAC[2], responseFrame->SenderMAC[3], 
			responseFrame->SenderMAC[4], responseFrame->SenderMAC[5], responseFrame->SenderIP[0], responseFrame->SenderIP[1], 
			responseFrame->SenderIP[2], responseFrame->SenderIP[3]);

	// Return new frame
	return frame;
}