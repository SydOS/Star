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

static uint32_t lapic_read(uint16_t offset) {
    // Read value from LAPIC.
    return *(volatile uint32_t*)(LAPIC_ADDRESS + offset);
}

static void lapic_write(uint16_t offset, uint32_t value) {
    // Send data to LAPIC.
    *(volatile uint32_t*)(LAPIC_ADDRESS + offset) = value;
    (void)lapic_read(LAPIC_REG_ID);
}

static lapic_icr_t create_empty_icr() {
    // Create empty ICR.
    lapic_icr_t icr;
    icr.vector = 0;
    icr.deliveryMode = 0;
    icr.destinationMode = 0;
    icr.deliveryStatus = 0;
    icr.reserved1 = 0;
    icr.level = 0;
    icr.triggerMode = 0;
    icr.reserved2 = 0;
    icr.destinationShorthand = 0;
    icr.reserved3 = 0;
    icr.destination = 0;

    return icr;
}

static void lapic_send_icr(lapic_icr_t icr) {
    // Get ICR into two 32-bit values.
    uint32_t *data = (uint32_t*)&icr;
    uint64_t *test = (uint64_t*)&icr;

    // Send ICR to LAPICs.
    lapic_write(LAPIC_REG_INTERRUPT_CMD_HIGH, data[1]);
    lapic_write(LAPIC_REG_INTERRUPT_CMD_LOW, data[0]);

    // Wait for signal from LAPIC.
    while (lapic_read(LAPIC_REG_INTERRUPT_CMD_LOW) & (LAPIC_DELIVERY_STATUS_SEND_PENDING << 12));
}

void lapic_send_init(uint8_t apic) {
    // Send INIT to specified APIC.
    lapic_icr_t icr = create_empty_icr();
    icr.deliveryMode = LAPIC_DELIVERY_INIT;
    icr.destinationMode = LAPIC_DEST_MODE_PHYSICAL;
    icr.triggerMode = LAPIC_TRIGGER_EDGE;
    icr.level = LAPIC_LEVEL_ASSERT;
    icr.destination = apic;

    // Send ICR.
    lapic_send_icr(icr);;
}

void lapic_send_startup(uint8_t apic, uint8_t vector) {
    // Send startup to specified APIC.
    lapic_icr_t icr = create_empty_icr();
    icr.vector = vector;
    icr.deliveryMode = LAPIC_DELIVERY_STARTUP;
    icr.destinationMode = LAPIC_DEST_MODE_PHYSICAL;
    icr.triggerMode = LAPIC_TRIGGER_EDGE;
    icr.level = LAPIC_LEVEL_ASSERT;
    icr.destination = apic;

    // Send ICR.
    lapic_send_icr(icr);
}

void lapic_send_nmi_all() {
    // Send NMI to all LAPICs but ourself.
    lapic_icr_t icr = create_empty_icr();
    icr.deliveryMode = LAPIC_DELIVERY_NMI;
    icr.triggerMode = LAPIC_TRIGGER_EDGE;
    icr.destinationShorthand = LAPIC_DEST_SHORTHAND_ALL_BUT_SELF;
    icr.level = LAPIC_LEVEL_ASSERT;

    // Send ICR.
    lapic_send_icr(icr);
}

uint32_t lapic_id() {
    // Get ID.
    return lapic_read(LAPIC_REG_ID) >> 24;
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

void lapic_setup() {
    // Map LAPIC and get info.
    kprintf("LAPIC: Mapped to 0x%X\n", LAPIC_ADDRESS);
    kprintf("LAPIC: x2 APIC: %s\n", lapic_x2apic() ? "yes" : "no");
    kprintf("LAPIC: ID: %u\n", lapic_id());
    kprintf("LAPIC: Version: 0x%x\n", lapic_version());
    kprintf("LAPIC: Max LVT entry: 0x%x\n", lapic_max_lvt());

    // Configure LAPIC.
    lapic_write(LAPIC_REG_TASK_PRIORITY, 0x00);
    lapic_write(LAPIC_REG_DEST_FORMAT, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LOGICAL_DEST, 1 << 24);
    lapic_create_spurious_interrupt(0xFF);
}

void lapic_init() {
    // Get the base address of the local APIC and map it.
    uint32_t base = lapic_get_base();
    paging_map_virtual_to_phys(LAPIC_ADDRESS, base);
    kprintf("LAPIC: Initializing LAPIC at 0x%X...\n", base);
    idt_set_gate(0xFF, (uint32_t)_isr_empty, 0x08, 0x8E);

    lapic_setup();

    // Create spurious interrupt.
    
    
    kprintf("LAPIC: Initialized!\n");
}
