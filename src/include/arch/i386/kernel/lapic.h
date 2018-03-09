#ifndef LAPIC_H
#define LAPIC_H

#define IA32_APIC_BASE_MSR          0x1B
#define IA32_APIC_BASE_MSR_X2APIC   0x400

#define LAPIC_BASE_ADDR_MASK 0xFFFFF000


extern bool lapic_supported();
extern void lapic_init();

#endif