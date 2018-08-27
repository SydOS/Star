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

bool isWaitingForResponse = false;
arp_frame_t* responseFrame;

void arp_process_response(ethernet_frame_t* ethFrame) {
	// Generate temporary frame
	arp_frame_t* inFrame = (arp_frame_t*)((uint8_t*)ethFrame+sizeof(ethernet_frame_t));

	// Check if we are waiting for an ARP reply and it is a reply
	if (isWaitingForResponse == true && swap_uint16(inFrame->Opcode) == 2) { // TODO replace with #define for opcode.
		// Set the global responseFrame variable to our inputted frame
		responseFrame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
		// idk
		memcpy(responseFrame, (arp_frame_t*)((uint8_t*)ethFrame+sizeof(ethernet_frame_t)), sizeof(arp_frame_t));
		// Set waiting for response to false so handling code will work
		isWaitingForResponse = false;
	}
}

arp_frame_t* arp_get_mac_address(net_device_t* netDevice, uint8_t* targetIP) {
	if (responseFrame == 0x0) {
		kprintf("ARP: Blank frame\n");
		// Allocate memory for frame
		responseFrame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));
		// Clear frame with 0s
		memset(responseFrame, 0, sizeof(arp_frame_t));
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
	isWaitingForResponse = true;
	netDevice->Send(netDevice, frame, frameSize);
	kheap_free(arpFrame);

	// Wait for a reply
	uint64_t targetTick = timer_ticks() + 2000;
	while (isWaitingForResponse &&
		   responseFrame->SenderIP[0] != targetIP[0] ||
		   responseFrame->SenderIP[1] != targetIP[1] ||
		   responseFrame->SenderIP[2] != targetIP[2] ||
		   responseFrame->SenderIP[3] != targetIP[3]) {
		if (timer_ticks() >= targetTick) {
			kprintf("ARP: request timeout\n");
			// Allocate memory for frame
			arp_frame_t *frame = (arp_frame_t*)kheap_alloc(sizeof(arp_frame_t));

			// Clear frame with 0s
			memset(frame, 0, sizeof(arp_frame_t));
			return frame;
		}
	}
	
	kprintf("ARP: IP for %2X:%2X:%2X:%2X:%2X:%2X is %u.%u.%u.%u\n",
			responseFrame->SenderMAC[0], responseFrame->SenderMAC[1], responseFrame->SenderMAC[2], responseFrame->SenderMAC[3], 
			responseFrame->SenderMAC[4], responseFrame->SenderMAC[5], responseFrame->SenderIP[0], responseFrame->SenderIP[1], 
			responseFrame->SenderIP[2], responseFrame->SenderIP[3]);

	// Return new frame
	return responseFrame;
}