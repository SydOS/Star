#include <main.h>
#include <string.h>
#include <kprint.h>

#include <kernel/memory/kheap.h>

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
	kprintf("L2Eth: frame size of 0x%X calculated, (Eth frame 0x%X | payloadSize 0x%X)\n", *FrameSize, sizeof(ethernet_frame_t), payloadSize);

	// Allocate the RAM for our new Ethernet frame to live in
	ethernet_frame_t *frame = (ethernet_frame_t*)kheap_alloc(*FrameSize);
	kprintf("L2Eth: allocated frame size of 0x%X\n", sizeof(ethernet_frame_t));
	// Clear out the frame's memory space
	memset(frame, 0, *FrameSize);
	kprintf("L2Eth: cleared frame with 0s\n");

	// Copy the 6 bytes a MAC address should be
	// This is the destination MAC address
	memcpy(frame->MACDest, MACDest, 6);
	kprintf("L2Eth: copied destination MAC address to 0x%X\n", &frame->MACDest);
	// This is the source MAC address
	memcpy(frame->MACSrc, MACSrc, 6);
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