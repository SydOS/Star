/*
 * File: networking.c
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
#include <kprint.h>
#include <string.h>
#include <kernel/networking/networking.h>

#include <kernel/lock.h>
#include <kernel/memory/kheap.h>
#include <kernel/tasking.h>

#include <kernel/networking/layers/l2-ethernet.h>
#include <kernel/networking/layers/l3-ipv4.h>
#include <kernel/networking/protocols/arp.h>

// Network device linked list.
net_device_t *NetDevices = NULL;


void dumphex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        kprintf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            kprintf(" ");
            if ((i+1) % 16 == 0) {
                kprintf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    kprintf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    kprintf("   ");
                }
                kprintf("|  %s \n", ascii);
            }
        }
    }
}

void networking_handle_packet(net_device_t *netDevice, void *data, uint16_t length) {
    // Create packet.
    net_packet_t *packet = (net_packet_t*)kheap_alloc(sizeof(net_packet_t));
    memset(packet, 0, sizeof(net_packet_t));

    // Set packet properties.
    packet->PacketData = data;
    packet->PacketLength = length;

    // Lock this code.
    spinlock_lock(&netDevice->CurrentRxPacketLock);

    // Add packet to end of queue.
    if (netDevice->LastRxPacket != NULL)
        netDevice->LastRxPacket->Next = packet;
    netDevice->LastRxPacket = packet;

    // If there is no packet currently being processed, make this one the current.
    if (netDevice->CurrentRxPacket == NULL)
        netDevice->CurrentRxPacket = packet;

    // Release lock.
    spinlock_release(&netDevice->CurrentRxPacketLock);
}

static void networking_packet_process_thread(net_device_t *netDevice) {
    while (true) {
        // Wait until we have a packet ready.
        while (netDevice->CurrentRxPacket == NULL);

        // Process packet here.
        kprintf("process\n");
        ethernet_frame_t *ethFrame = l2_ethernet_handle_packet(netDevice->CurrentRxPacket);
        dumphex(ethFrame, netDevice->CurrentRxPacket->PacketLength);

        // Lock this code.
        spinlock_lock(&netDevice->CurrentRxPacketLock);
        
        // Move to next packet.
        net_packet_t *currPacket = netDevice->CurrentRxPacket;
        netDevice->CurrentRxPacket = netDevice->CurrentRxPacket->Next;
        if (netDevice->CurrentRxPacket == NULL)
            netDevice->LastRxPacket = NULL;

        // Release lock.
        spinlock_release(&netDevice->CurrentRxPacketLock);

        // Free packet.
        kheap_free(currPacket->PacketData);
        kheap_free(currPacket);
    }
}

void networking_register_device(net_device_t *netDevice) {
    // If there aren't any devices at all, add as first device.
    if (NetDevices == NULL) {
        NetDevices = netDevice;
        netDevice->Next = netDevice;
        netDevice->Prev = netDevice;
    }
    else { // Add to end of list.
        netDevice->Next = NetDevices;
        NetDevices->Prev->Next = netDevice;
        netDevice->Prev = NetDevices->Prev;
        NetDevices->Prev = netDevice;
    }
    kprintf("NET: Registered device %s!\n", netDevice->Name != NULL ? netDevice->Name : "unknown");

    // Start up packet reception thread.
    tasking_thread_schedule_proc(tasking_thread_create_kernel("net_worker", networking_packet_process_thread, (uintptr_t)netDevice, 0, 0), 0);

    // Create some variables to prepare for our ARP request
    uint16_t frameSize;
    uint8_t targetIP[NET_IPV4_LENGTH];
    targetIP[0] = 192;
    targetIP[1] = 168;
    targetIP[2] = 1;
    targetIP[3] = 1;

    // Generate and send ARP request
    ethernet_frame_t* frame = arp_create_packet(netDevice, targetIP, &frameSize);
    netDevice->Send(netDevice, frame, frameSize);
    kprintf("NET: SENT TEST PACKET\n");
    dumphex(frame, frameSize);
    kheap_free(frame);
}

void networking_print_devices(void) {
    net_device_t *netDevice = NetDevices;
    net_device_t *firstDevice = NetDevices;
    kprintf("NET: List of networking devices:\n");
    while (netDevice != NULL) {
        kprintf("NET:    Device %s\n", netDevice->Name != NULL ? netDevice->Name : "unknown");

        // Move to next device.
        netDevice = netDevice->Next;
        if (netDevice == firstDevice)
            break;
    }
}

void networking_init(void) {
    net_device_t *netDevice = NetDevices;


}
