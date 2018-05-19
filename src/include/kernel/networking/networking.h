/*
 * File: networking.h
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

#ifndef NETWORKING_H
#define NETWORKING_H

#include <main.h>

typedef struct net_packet_t {
    // Next packet in linked list, or NULL for last packet.
    struct net_packet_t *Next;

    // The actual packet.
    void *PacketData;
    uint16_t PacketLength;
} net_packet_t;

typedef struct net_device_t {
    // Relationship to other devices in linked list.
    struct net_device_t *Next;
    struct net_device_t *Prev;

    // Driver object and friendly name.
    void *Device;
    char *Name;

    // Function pointers.
    bool (*Reset)(struct net_device_t *netDevice);
    bool (*Send)(struct net_device_t *netDevice, void *data, uint16_t length);

    // Current and last packet in reception queue.
    net_packet_t *CurrentRxPacket;
    net_packet_t *LastRxPacket;
} net_device_t;

// Linked list of networking devices.
extern net_device_t *NetDevices;

extern void networking_handle_packet(net_device_t *netDevice, void *data, uint16_t length);

extern void networking_register_device(net_device_t *netDevice);
extern void networking_print_devices(void);

extern void networking_init(void);

#endif
