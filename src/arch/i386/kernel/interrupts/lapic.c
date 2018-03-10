#include <main.h>
#include <kprint.h>
#include <io.h>
#include <arch/i386/kernel/lapic.h>
#include <arch/i386/kernel/cpuid.h>
#include <arch/i386/kernel/pic.h>
#include <kernel/paging.h>

extern void _isr_empty();


bool lapic_supported() {
    // Check for the APIC feature.
    uint32_t result, unused;
    if (cpuid_query(CPUID_GETFEATURES, &unused, &unused, &unused, &result))
        return result & CPUID_FEAT_EDX_APIC;

    return false;
}

uint32_t lapic_get_base() {
    // Get LAPIC base physical address.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & LAPIC_BASE_ADDR_MASK);
}

bool lapic_x2apic() {
    // Determine if LAPIC is an x2APIC.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & IA32_APIC_BASE_MSR_X2APIC);
}

bool lapic_enabled() {
    // Determine if LAPIC is enabled.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & IA32_APIC_BASE_MSR_ENABLE);
}

uint32_t lapic_read(uint16_t offset) {
    // Read value from LAPIC.
    return *(volatile uint32_t*)(LAPIC_ADDRESS + offset);
}

void lapic_write(uint16_t offset, uint32_t value) {
    // Send data to LAPIC.
    *(volatile uint32_t*)(LAPIC_ADDRESS + offset) = value;
}

uint32_t lapic_id() {
    // Get ID.
    return lapic_read(LAPIC_REG_ID);
}

uint8_t lapic_version() {
    // Get version.
    return (uint8_t)(lapic_read(LAPIC_REG_VERSION) & 0xFF);
}

uint8_t lapic_max_lvt() {
    // Get max LVTs.
    return (uint8_t)((lapic_read(LAPIC_REG_VERSION) >> 16) & 0xFF);
}

void lapic_eoi() {
    // Send EOI to LAPIC.
    lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_create_spurious_interrupt(uint8_t interrupt) {
    // Create spurious vector.
    lapic_write(LAPIC_REG_SPURIOUS_INT_VECTOR, interrupt | 0x100);
}

void lapic_init() {
    // Get the base address of the local APIC.  
    uint32_t base = lapic_get_base();
    kprintf("LAPIC: Initializing LAPIC at 0x%X...\n", base);

    // Map LAPIC and get info.
    paging_map_virtual_to_phys(LAPIC_ADDRESS, base);
    kprintf("LAPIC: Mapped to 0x%X\n", LAPIC_ADDRESS);
    kprintf("LAPIC: x2 APIC: %s\n", lapic_x2apic() ? "yes" : "no");
    kprintf("LAPIC: ID: %u\n", lapic_id());
    kprintf("LAPIC: Version: 0x%x\n", lapic_version());
    kprintf("LAPIC: Max LVT entry: 0x%x\n", lapic_max_lvt());

    // Configure LAPIC.
    lapic_write(LAPIC_REG_TASK_PRIORITY, 0x00);
    lapic_write(LAPIC_REG_DEST_FORMAT, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LOGICAL_DEST, 1 << 24);

    // Create spurious interrupt.
    idt_set_gate(0xFF, (uint32_t)_isr_empty, 0x08, 0x8E);
    lapic_create_spurious_interrupt(0xFF);
    kprintf("LAPIC: Initialized!\n");
}
