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

#include <kernel/memory/kheap.h>

// Network device linked list.
net_device_t *NetDevices = NULL;

void networking_handle_packet(net_device_t *netDevice, void *data, uint16_t length) {
    // Create packet.
    net_packet_t *packet = (net_packet_t*)kheap_alloc(sizeof(net_packet_t));
    memset(packet, 0, sizeof(net_packet_t));

    // Set packet properties.
    packet->PacketData = data;
    packet->PacketLength = length;

    // Add packet to end of queue.
    if (netDevice->LastRxPacket != NULL)
        netDevice->LastRxPacket->Next = packet;
    netDevice->LastRxPacket = packet;

    // If there is no packet currently being processed, make this one the current.
    if (netDevice->CurrentRxPacket == NULL)
        netDevice->CurrentRxPacket = packet;
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

    // This is where our test packet stuff will be for now.
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
