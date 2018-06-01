/*
 * File: l2-ipv4.h
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

#ifndef NETWORKING_IPV4_H
#define NETWORKING_IPV4_H

#include <main.h>

typedef struct {
	uint8_t Version : 4;
    uint8_t IHL : 4;
    uint8_t DSCP : 6;
    uint8_t ECN : 2;
    uint16_t TotalLength;
    uint16_t Identification;
    uint8_t Flags : 3;
    uint16_t FragmentationOffset : 13;
    uint8_t TimeToLive;
    uint8_t Protocol;
    uint16_t HeaderChecksum;
    uint8_t SourceIP[4];
    uint8_t DestinationIP[4];

	// The payload comes after, but can be variable length, so just allocate
	// the memory size to be bigger than this actual struct to fit it.
} __attribute__((packed)) ipv4_frame_t;

extern ipv4_frame_t* l3_ipv4_create_frame(uint8_t* SourceIP, uint8_t* DestinationIP, 
                                   uint8_t protocol, uint16_t payloadSize, 
                                   void* payloadPointer, uint16_t *FrameSize);

#endif