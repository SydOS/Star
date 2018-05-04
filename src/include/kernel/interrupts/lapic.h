#ifndef LAPIC_H
#define LAPIC_H

#include <main.h>

// Address that the local APIC is mapped to.

#ifdef X86_64
#define LAPIC_BASE_ADDR_MASK    0xFFFFFFFFFFFFF000
#else
#define LAPIC_BASE_ADDR_MASK    0xFFFFF000
#endif

// LAPIC MSRs.
#define IA32_APIC_BASE_MSR              0x1B
#define IA32_APIC_BASE_MSR_X2APIC       0x400
#define IA32_APIC_BASE_MSR_ENABLE       0x800

// LAPIC registers.
#define LAPIC_REG_ID                    0x20
#define LAPIC_REG_VERSION               0x30
#define LAPIC_REG_TASK_PRIORITY         0x80
#define LAPIC_REG_ARB_PRIORITY          0x90
#define LAPIC_REG_PROC_PRIORITY         0xA0
#define LAPIC_REG_EOI                   0xB0
#define LAPIC_REG_REMOTE_READ           0xC0
#define LAPIC_REG_LOGICAL_DEST          0xD0
#define LAPIC_REG_DEST_FORMAT           0xE0
#define LAPIC_REG_SPURIOUS_INT_VECTOR   0xF0

#define LAPIC_REG_INSERVICE             0x100
#define LAPIC_REG_TRIGGER_MODE          0x180
#define LAPIC_REG_IRR                   0x200
#define LAPIC_REG_ERROR_STATUS          0x280

#define LAPIC_REG_INTERRUPT_CMD_LOW     0x300
#define LAPIC_REG_INTERRUPT_CMD_HIGH    0x310
#define LAPIC_REG_LVT_TIMER             0x320
#define LAPIC_REG_LVT_THERMAL           0x330
#define LAPIC_REG_LVT_PERF              0x340
#define LAPIC_REG_LVT_LINT0             0x350
#define LAPIC_REG_LVT_LINT1             0x360
#define LAPIC_REG_LVT_ERROR             0x370
#define LAPIC_REG_TIMER_INITIAL         0x380
#define LAPIC_REG_TIMER_CURRENT         0x390
#define LAPIC_REG_TIMER_DIVIDE          0x3E0

#define LAPIC_SPURIOUS_INT              0xFF

#define LAPIC_TIMER_MASKED          0x10000        

#define LAPIC_TIMER_MODE_ONESHOT    0x00000
#define LAPIC_TIMER_MODE_PERIODIC   0x20000
#define LAPIC_TIMER_MODE_TSC        0x40000

#define LAPIC_TIMER_DIVIDE2         0x0
#define LAPIC_TIMER_DIVIDE4         0x1
#define LAPIC_TIMER_DIVIDE8         0x2
#define LAPIC_TIMER_DIVIDE16        0x3

// Delivery mode.
enum LAPIC_DELIVERY_MODE {
    // Delivers the interrupt specified in the vector field to the target processor or processors.
    LAPIC_DELIVERY_FIXED    = 0x0,

    // Same as fixed mode, except that the interrupt is delivered to the processor executing at the
    // lowest priority among the set of processors specified in the destination field.
    LAPIC_DELIVERY_LOWEST   = 0x1,

    // Delivers an SMI interrupt to the target processor or processors.
    LAPIC_DELIVERY_SMI      = 0x2,

    // Delivers an NMI interrupt to the target processor or processors.
    LAPIC_DELIVERY_NMI      = 0x4,

    // Delivers an INIT request to the target processor or processors, which causes them to perform an INIT.
    LAPIC_DELIVERY_INIT     = 0x5,

    // (Not supported in the Pentium 4 and Intel Xeon processors.) Sends a synchronization message to all the
    // local APICs in the system to set their arbitration IDs (stored in their Arb ID registers) to the values of their APIC IDs
    LAPIC_DELIVERY_INIT_DE  = 0x5,

    // Sends a special “start-up” IPI (called a SIPI) to the target processor or processors.
    LAPIC_DELIVERY_STARTUP  = 0x6
};

// Destination mode.
enum LAPIC_DEST_MODE {
    LAPIC_DEST_MODE_PHYSICAL    = 0,
    LAPIC_DEST_MODE_LOGICAL     = 1
};

// Delivery status.
enum LAPIC_DELIVERY_STATUS {
    LAPIC_DELIVERY_STATUS_IDLE          = 0, // Indicates that this local APIC has completed sending any previous IPIs.
    LAPIC_DELIVERY_STATUS_SEND_PENDING  = 1  // Indicates that this local APIC has not completed sending the last IPI.
};

// Level.
enum LAPIC_LEVEL {
    LAPIC_LEVEL_DEASSERT    = 0,
    LAPIC_LEVEL_ASSERT      = 1
};

// Trigger mode.
enum LAPIC_TRIGGER_MODE {
    LAPIC_TRIGGER_EDGE      = 0,
    LAPIC_TRIGGER_LEVEL     = 1
};

// Destination shorthand.
enum LAPIC_DEST_SHORTHAND {
    LAPIC_DEST_SHORTHAND_NONE           = 0x0,
    LAPIC_DEST_SHORTHAND_SELF           = 0x1,
    LAPIC_DEST_SHORTHAND_ALL            = 0x2,
    LAPIC_DEST_SHORTHAND_ALL_BUT_SELF   = 0x3
};

// LAPIC ICR packet.
typedef struct {
    // The vector number of the interrupt being sent.
    uint8_t Vector                  : 8;

    // Specifies the type of IPI to be sent. This field is also know as the IPI message type field.
    uint8_t DeliveryMode            : 3;

    // Selects either physical (0) or logical (1) destination mode.
    uint8_t DestinationMode         : 1;

    // Indicates the IPI delivery status.
    uint8_t DeliveryStatus          : 1;

    // Reserved.
    uint8_t Reserved1               : 1;

    // For the INIT level de-assert delivery mode this flag must be set to 0; for all other delivery
    // modes it must be set to 1. (This flag has no meaning in Pentium 4 and Intel Xeon processors,
    // and will always be issued as a 1.)
    uint8_t Level                   : 1;

    // Selects the trigger mode when using the INIT level de-assert delivery mode: edge (0) or level (1).
    // It is ignored for all other delivery modes. (This flag has no meaning in Pentium 4 and Intel Xeon processors,
    // and will always be issued as a 0.)
    uint8_t TriggerMode             : 1;

    // Reserved.
    uint8_t Reserved2               : 2;

    // Indicates whether a shorthand notation is used to specify the destination of the interrupt and,
    // if so, which shorthand is used.
    uint8_t DestinationShorthand    : 2;
    
    // Reserved.
    uint64_t Reserved3              : 36;

    // Specifies the target processor or processors. This field is only used when the destination
    // shorthand field is set to 00B.
    uint8_t Destination             : 8;
} __attribute__((packed)) lapic_icr_t;

extern bool lapic_supported(void);
extern bool lapic_x2apic(void);
extern bool lapic_enabled(void);
extern void lapic_send_init(uint8_t apic);
extern void lapic_send_startup(uint8_t apic, uint8_t vector);

extern uint32_t lapic_timer_get_rate(void);
extern void lapic_timer_start(uint32_t rate);

extern uint32_t lapic_id(void);
extern uint8_t lapic_version(void);
extern uint8_t lapic_max_lvt(void);
extern void lapic_eoi(void);
extern int16_t lapic_get_irq(void);
extern void lapic_setup(void);
extern void lapic_init(void);

#endif
