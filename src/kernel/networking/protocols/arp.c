#include <main.h>
#include <string.h>

#include <kernel/memory/kheap.h>
#include <kernel/networking/protocols/arp.h>

uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

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
	memcpy((frame->SenderMAC), SenderMAC, 6);
	for (int x = 0; x < 4; x++)
        frame->SenderIP[x] = 0x00;
    for (int x = 0; x < 6; x++)
        frame->TargetMAC[x] = 0x00;
    memcpy((frame->TargetIP), TargetIP, 4);

    return frame;
}