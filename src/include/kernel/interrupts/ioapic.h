/*
 * File: ioapic.h
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

#ifndef IOAPIC_H
#define IOAPIC_H

#include <main.h>

// I/O APIC memory registers.
#define IOAPIC_IOREGSL      0x00 // I/O Register Select (index).
#define IOAPIC_IOWIN        0x10 // I/O Window (data).

// I/O APIC registers.
#define IOAPIC_REG_ID       0x00
#define IOAPIC_REG_VERSION  0x01
#define IOAPIC_REG_ARB      0x02
#define IOAPIC_REG_REDTBL   0x10

// Delivery mode.
enum IOAPIC_DELIVERY_MODE {
    IOAPIC_DELIVERY_FIXED   = 0x0,
    IOAPIC_DELIVERY_LOWEST  = 0x1,
    IOAPIC_DELIVERY_SMI     = 0x2,
    IOAPIC_DELIVERY_NMI     = 0x4,
    IOAPIC_DELIVERY_INIT    = 0x5,
    IOAPIC_DELIVERY_EXT_INT = 0x7
};

// Destination mode.
enum IOAPIC_DEST_MODE {
    IOAPIC_DEST_MODE_PHYSICAL   = 0,
    IOAPIC_DEST_MODE_LOGICAL    = 1
};

// Delivery status.
enum IOAPIC_DELIVERY_STATUS {
    IOAPIC_DELIVERY_STATUS_IDLE         = 0,
    IOAPIC_DELIVERY_STATUS_SEND_PENDING = 1
};

// I/O APIC redirection entry.
struct ioapic_redirection_entry {
    // The vector field is an 8 bit field containing the interrupt vector for this interrupt.
    uint8_t interruptVector         : 8;

    // The Delivery Mode is a 3 bit field that specifies how the APICs listed in the
    // destination field should act upon reception of this signal.
    uint8_t deliveryMode            : 3;

    // This field determines the interpretation of the Destination field.
    // 0=Physical, 1=Logical.
    uint8_t destinationMode         : 1;

    // The Delivery Status bit contains the current status of the delivery of this interrupt.
    uint8_t deliveryStatus          : 1;

    // This bit specifies the polarity of the interrupt signal.
    // 0=High active, 1=Low active.
    uint8_t interruptInputPolarity  : 1;

    // This bit is used for level triggered interrupts.
    uint8_t remoteIrr               : 1;

    // The trigger mode field indicates the type of signal on the interrupt pin that triggers an interrupt.
    // 1=Level sensitive, 0=Edge sensitive.
    uint8_t triggerMode             : 1;

    // When this bit is 1, the interrupt signal is masked.
    bool interruptMask              : 1;
    uint64_t reserved               : 39;

    // If the Destination Mode of this entry is Physical Mode (bit 11=0), bits contain an APIC ID.
    // If Logical Mode is selected (bit 11=1), the Destination Field potentially defines a set of processors.
    uint8_t destinationField        : 8;
} __attribute__((packed));
typedef struct ioapic_redirection_entry ioapic_redirection_entry_t;

extern uint32_t ioapic_remap_interrupt(uint32_t interrupt);
extern bool ioapic_supported(void);
extern uint8_t ioapic_id(void);
extern uint8_t ioapic_version(void);
extern uint8_t ioapic_max_interrupts(void);
extern void ioapic_enable_interrupt(uint8_t interrupt, uint8_t vector);
extern void ioapic_init(void);

#endif
