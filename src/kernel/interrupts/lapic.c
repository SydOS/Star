#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/interrupts/lapic.h>

#include <kernel/cpuid.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/irqs.h>
#include <kernel/memory/paging.h>

extern void _irq_empty(void);
static void *lapicPointer;

bool lapic_supported(void) {
    // Check for the APIC feature.
    uint32_t result, unused;
    if (cpuid_query(CPUID_GETFEATURES, &unused, &unused, &unused, &result))
        return result & CPUID_FEAT_EDX_APIC;

    return false;
}

uint32_t lapic_get_base(void) {
    // Get LAPIC base physical address.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & LAPIC_BASE_ADDR_MASK);
}

bool lapic_x2apic(void) {
    // Determine if LAPIC is an x2APIC.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & IA32_APIC_BASE_MSR_X2APIC);
}

bool lapic_enabled(void) {
    // Determine if LAPIC is enabled.
    return (((uint32_t)cpu_msr_read(IA32_APIC_BASE_MSR)) & IA32_APIC_BASE_MSR_ENABLE);
}

static uint32_t lapic_read(uint16_t offset) {
    // Read value from LAPIC.
    return *(volatile uint32_t*)((uintptr_t)(lapicPointer + offset));
}

static void lapic_write(uint16_t offset, uint32_t value) {
    // Send data to LAPIC.
    *(volatile uint32_t*)((uintptr_t)(lapicPointer + offset)) = value;
    (void)lapic_read(LAPIC_REG_ID);
}

static void lapic_send_icr(lapic_icr_t icr) {
    // Get ICR into two 32-bit values.
    uint32_t *data = (uint32_t*)&icr;

    // Send ICR to LAPICs.
    lapic_write(LAPIC_REG_INTERRUPT_CMD_HIGH, data[1]);
    lapic_write(LAPIC_REG_INTERRUPT_CMD_LOW, data[0]);

    // Wait for signal from LAPIC.
    while (lapic_read(LAPIC_REG_INTERRUPT_CMD_LOW) & (LAPIC_DELIVERY_STATUS_SEND_PENDING << 12));
}

void lapic_send_init(uint8_t apic) {
    // Send INIT to specified APIC.
    lapic_icr_t icr = {};
    icr.DeliveryMode = LAPIC_DELIVERY_INIT;
    icr.DestinationMode = LAPIC_DEST_MODE_PHYSICAL;
    icr.TriggerMode = LAPIC_TRIGGER_EDGE;
    icr.Level = LAPIC_LEVEL_ASSERT;
    icr.Destination = apic;

    // Send ICR.
    lapic_send_icr(icr);;
}

void lapic_send_startup(uint8_t apic, uint8_t vector) {
    // Send startup to specified APIC.
    lapic_icr_t icr = {};
    icr.Vector = vector;
    icr.DeliveryMode = LAPIC_DELIVERY_STARTUP;
    icr.DestinationMode = LAPIC_DEST_MODE_PHYSICAL;
    icr.TriggerMode = LAPIC_TRIGGER_EDGE;
    icr.Level = LAPIC_LEVEL_ASSERT;
    icr.Destination = apic;

    // Send ICR.
    lapic_send_icr(icr);
}

void lapic_send_nmi_all(void) {
    // Send NMI to all LAPICs but ourself.
    lapic_icr_t icr = {};
    icr.DeliveryMode = LAPIC_DELIVERY_NMI;
    icr.TriggerMode = LAPIC_TRIGGER_EDGE;
    icr.DestinationShorthand = LAPIC_DEST_SHORTHAND_ALL_BUT_SELF;
    icr.Level = LAPIC_LEVEL_ASSERT;

    // Send ICR.
    lapic_send_icr(icr);
}

uint32_t lapic_timer_get_rate(void) {
    // Set divider to 16.
    kprintf("LAPIC: Calculating tick rate for timer...\n");
    lapic_write(LAPIC_REG_TIMER_DIVIDE, LAPIC_TIMER_DIVIDE16);

    uint32_t averageCount = 0;
    for (uint8_t i = 0; i < 20; i++) {
        // Reset initial count to 0xFFFFFFFF.
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0xFFFFFFFF);
        lapic_write(LAPIC_REG_TIMER_CURRENT, 0xFFFFFFFF);

        // Wait for 100 ms and get current count.
        sleep(100);
        averageCount += 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURRENT);
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0);
        lapic_write(LAPIC_REG_TIMER_CURRENT, 0);
    }  

    averageCount = averageCount / 20;
    kprintf("LAPIC: Timer ticked %u times on average in 100ms.\n", averageCount);
    return averageCount / 100;
}

void lapic_timer_start(uint32_t rate) {
    // Start up timer using calculated amount.
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MODE_PERIODIC | (IRQ_OFFSET + IRQ_TIMER));
    lapic_write(LAPIC_REG_TIMER_DIVIDE, LAPIC_TIMER_DIVIDE16);
    lapic_write(LAPIC_REG_TIMER_INITIAL, rate);
}

uint32_t lapic_id(void) {
    // Get ID if LAPIC is configured, otherwise return 0.
    return lapicPointer != NULL ? lapic_read(LAPIC_REG_ID) >> 24 : 0;
}

uint8_t lapic_version(void) {
    // Get version.
    return (uint8_t)(lapic_read(LAPIC_REG_VERSION) & 0xFF);
}

uint8_t lapic_max_lvt(void) {
    // Get max LVTs.
    return (uint8_t)((lapic_read(LAPIC_REG_VERSION) >> 16) & 0xFF);
}

void lapic_eoi(void) {
    // Send EOI to LAPIC.
    lapic_write(LAPIC_REG_EOI, 0);
}

int16_t lapic_get_irq(void) {
    // Check each ISR register for a bit set.
    for (uint8_t i = 0; i < 8; i++) {
        // Read In-Service register and check for bits.
        uint32_t isr = lapic_read(LAPIC_REG_INSERVICE + (i * 0x10));
        if (isr)
            return ((i * 32) + __builtin_ctz(isr)) - IRQ_OFFSET;
    }

    // No IRQs are set.
    return -1;
}

void lapic_setup(void) {
    // Map LAPIC and get info.
    kprintf("LAPIC: x2 APIC: %s\n", lapic_x2apic() ? "yes" : "no");
    kprintf("LAPIC: ID: %u\n", lapic_id());
    kprintf("LAPIC: Version: 0x%x\n", lapic_version());
    kprintf("LAPIC: Max LVT entry: 0x%x\n", lapic_max_lvt());

    // Configure LAPIC.
    lapic_write(LAPIC_REG_TASK_PRIORITY, 0x00);
    lapic_write(LAPIC_REG_DEST_FORMAT, 0xFFFFFFFF);
    lapic_write(LAPIC_REG_LOGICAL_DEST, 1 << 24);

    // Create spurious interrupt.
    lapic_write(LAPIC_REG_SPURIOUS_INT_VECTOR, LAPIC_SPURIOUS_INT | 0x100);
}

void lapic_init(void) {
    // Get the base address of the local APIC and map it.
    uint32_t base = lapic_get_base();
    lapicPointer = paging_device_alloc(base, base);
    
    kprintf("LAPIC: Mapped LAPIC at 0x%X to 0x%p...\n", base, lapicPointer);
    //idt_open_interrupt_gate(LAPIC_SPURIOUS_INT, (uintptr_t)_irq_empty);

    lapic_setup();
    kprintf("LAPIC: Initialized!\n");
}
