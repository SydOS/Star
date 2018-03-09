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
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & LAPIC_BASE_ADDR_MASK);
}

bool lapic_x2apic() {
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & IA32_APIC_BASE_MSR_X2APIC);
}

uint32_t lapic_reg_read(uint32_t base, uint32_t reg) {
    return *(volatile uint32_t*)(base + reg);
}

void lapic_reg_write(uint32_t base, uint32_t reg, uint32_t value) {
    *(volatile uint32_t*)(base + reg) = value;
}

void lapic_init() {
    // Disable PIC.
    pic_disable();
    
    // Get the base address of the local APIC.
    uint32_t base = lapic_get_base();
    kprintf("Local APIC base addresss: 0x%X\n", base);
    if (lapic_x2apic())
        kprintf("Local APIC is an x2 APIC!\n");

    // Map address.
    paging_map_virtual_to_phys(base, base);
    uint32_t version = lapic_reg_read(base, 0x30);
    kprintf("ID: 0x%x\n", lapic_reg_read(base, 0x20));
    kprintf("Version: 0x%x\n", version & 0x000000FF);
    kprintf("Max LVT entry: 0x%x\n", version & 0x00FF0000);

    lapic_reg_write(base, 0x80, 0x00);
    lapic_reg_write(base, 0xE0, 0xFFFFFFFF);
    lapic_reg_write(base, 0xD0, 1 << 24);

    // Enable interrupts?
    idt_set_gate(0xFF, (uint32_t)_isr_empty, 0x08, 0x8E);
    lapic_reg_write(base, 0xF0, 0xFF | 0x100);

    
}