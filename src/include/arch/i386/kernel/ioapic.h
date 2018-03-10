#ifndef IOAPIC_H
#define IOAPIC_H

#include <main.h>

// Address that the I/O APIC is mapped to.
#define IOAPIC_ADDRESS  0xFF0A0000

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

// I/O APIC redirection entry.
struct ioapic_redirection_entry {
    uint8_t interruptVector         : 8;
    uint8_t deliveryMode            : 3;
    bool destinationLogical         : 1; // Destination Mode. False = physical; true = logical.
    uint8_t deliveryStatus          : 1;
    uint8_t interruptInputPolarity  : 1;
    uint8_t remoteIrr               : 1;
    uint8_t triggerMode             : 1;
    bool interruptMask              : 1;
    uint64_t reserved               : 39;
    uint8_t destinationField        : 8;
} __attribute__((packed));
typedef struct ioapic_redirection_entry ioapic_redirection_entry_t;

extern uint8_t ioapic_id();
extern uint8_t ioapic_version();
extern uint8_t ioapic_max_interrupts();
extern void ioapic_enable_interrupt(uint8_t interrupt, uint8_t vector);
extern void ioapic_init(uintptr_t base);

#endif
