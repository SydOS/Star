#ifndef LAPIC_H
#define LAPIC_H

#include <main.h>

// Address that the local APIC is mapped to.
#define LAPIC_ADDRESS 0xFF0B0000
#define LAPIC_BASE_ADDR_MASK 0xFFFFF000

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

#define LAPIC_REG_INTERRUPT_CMD1        0x300
#define LAPIC_REG_INTERRUPT_CMD2        0x310
#define LAPIC_REG_LVT_TIMER             0x320
#define LAPIC_REG_LVT_THERMAL           0x330
#define LAPIC_REG_LVT_PERF              0x340
#define LAPIC_REG_LVT_LINT0             0x350
#define LAPIC_REG_LVT_LINT1             0x360
#define LAPIC_REG_LVT_ERROR             0x370
#define LAPIC_REG_TIMER_INITIAL_COUNT   0x380
#define LAPIC_REG_TIMER_CURRENT_COUNT   0x390
#define LAPIC_REG_TIMER_DIVIDE_CONF     0x3E0

extern bool lapic_supported();
extern bool lapic_enabled();
extern void lapic_eoi();
extern void lapic_init();

#endif
