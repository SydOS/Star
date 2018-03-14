#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/acpi/acpi.h>
#include <arch/i386/kernel/ioapic.h>
#include <arch/i386/kernel/interrupts.h>
#include <kernel/memory/paging.h>

// https://wiki.osdev.org/IOAPIC

static bool ioApicInitialized = false;

#define INTERRUPT_MAP_MAGIC 0xDEADBEEF
static uint32_t interrupt_redirections[IRQ_COUNT] = {
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC,
    INTERRUPT_MAP_MAGIC
};

uint32_t ioapic_remap_interrupt(uint32_t interrupt) {
    // Does a mapping exist for the interrupt?
    if (interrupt_redirections[interrupt] != INTERRUPT_MAP_MAGIC)
        return interrupt_redirections[interrupt];
    return interrupt;
}

inline bool ioapic_supported() {
    return ioApicInitialized;
}

static void ioapic_write(uint8_t offset, uint32_t value) {
    // Fill the I/O APIC's register selection memory area with our requested register offset.
    *(volatile uint32_t*)(IOAPIC_ADDRESS + IOAPIC_IOREGSL) = offset;

    // Write data into the I/O APIC's data window memory area.
    *(volatile uint32_t*)(IOAPIC_ADDRESS + IOAPIC_IOWIN) = value;
}

static uint32_t ioapic_read(uint8_t offset) {
    // Fill the I/O APIC's register selection memory area with our requested register offset.
    *(volatile uint32_t*)(IOAPIC_ADDRESS + IOAPIC_IOREGSL) = offset;

    // Read the result from the I/O APIC's data window memory area.
    return *(volatile uint32_t*)(IOAPIC_ADDRESS + IOAPIC_IOWIN);
}

uint8_t ioapic_id() {
    return (uint8_t)((ioapic_read(IOAPIC_REG_ID) >> 24) & 0xF);
}

uint8_t ioapic_version() {
    return (uint8_t)(ioapic_read(IOAPIC_REG_VERSION) & 0xFF);
}

uint8_t ioapic_max_interrupts() {
    return (uint8_t)(((ioapic_read(IOAPIC_REG_VERSION) >> 16) & 0xFF) + 1);
}

ioapic_redirection_entry_t ioapic_get_redirection_entry(uint8_t interrupt) {
    // Determine register offset.
    uint8_t offset = IOAPIC_REG_REDTBL + (interrupt * 2);

    // Get entry from I/O APIC.
    uint64_t data = (uint64_t)ioapic_read(offset) | ((uint64_t)ioapic_read(offset + 1) << 32);
    return *(ioapic_redirection_entry_t*)&data;
}

void ioapic_set_redirection_entry(uint8_t interrupt, ioapic_redirection_entry_t entry) {
    // Determine register offset and get pointer to entry.
    uint8_t offset = IOAPIC_REG_REDTBL + (interrupt * 2);
    uint32_t *data = (uint32_t*)&entry;
    
    // Send data to I/O APIC.
    ioapic_write(offset, data[0]);
    ioapic_write(offset + 1, data[1]);
}

void ioapic_enable_interrupt(uint8_t interrupt, uint8_t vector) {
    // Get entry for interrupt.
    ioapic_redirection_entry_t entry = ioapic_get_redirection_entry(interrupt);

    // Set entry fields.
    entry.interruptVector = vector;
    entry.deliveryMode = IOAPIC_DELIVERY_FIXED;
    entry.destinationMode = IOAPIC_DEST_MODE_PHYSICAL;
    entry.interruptMask = false;
    entry.destinationField = 0;

    // Save entry to I/O APIC.
    ioapic_set_redirection_entry(interrupt, entry);
    kprintf("IOAPIC: Mapped interrupt %u to 0x%X\n", interrupt, vector);
}

void ioapic_disable_interrupt(uint8_t interrupt) {
    // Get entry for interrupt and mask it.
    ioapic_redirection_entry_t entry = ioapic_get_redirection_entry(interrupt);
    entry.interruptMask = true;

    // Save entry to I/O APIC.
    ioapic_set_redirection_entry(interrupt, entry);
}

void ioapic_init() {
    // Only initialize once.
    if (ioApicInitialized)
        panic("IOAPIC: Attempting to initialize multiple times.\n");

    // Search for I/O APIC entry in ACPI.
    acpi_madt_entry_io_apic_t *ioApicMadt = (acpi_madt_entry_io_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_IO_APIC, 12, 0);
    if (ioApicMadt == NULL) {
        kprintf("IOAPIC: No I/O APIC found or ACPI is disabled. Aborting.\n");
        ioApicInitialized = false;
        return;
    }

    // Map I/O APIC to virtual memory.
    kprintf("IOAPIC: Initializing I/O APIC %u at 0x%X...\n", ioApicMadt->ioApicId, ioApicMadt->ioApicAddress);
    paging_map_virtual_to_phys(IOAPIC_ADDRESS, ioApicMadt->ioApicAddress);

    // Get info about I/O APIC.
    uint8_t maxInterrupts = ioapic_max_interrupts();
    kprintf("IOAPIC: Mapped I/O APIC to 0x%X!\n", IOAPIC_ADDRESS);
    kprintf("IOAPIC:     ID: %u\n", ioapic_id());
    kprintf("IOAPIC:     Version: 0x%X.\n", ioapic_version());
    kprintf("IOPAIC:     Max interrupts: %u\n", maxInterrupts);

    // Disable all interrupts.
    for (uint8_t i = 0; i < maxInterrupts; i++)
        ioapic_disable_interrupt(i);

    // Search for any interrupt remappings.
    acpi_madt_entry_interrupt_override_t *override = (acpi_madt_entry_interrupt_override_t*)acpi_search_madt(ACPI_MADT_STRUCT_INTERRUPT_OVERRIDE, 10, 0);
    while (override != NULL) {
        // Add override.
        kprintf("IOAPIC: Mapping interrupt %u to interrupt %u.\n", override->source, override->globalSystemInterrupt);
        interrupt_redirections[override->source] = override->globalSystemInterrupt;
        override = (acpi_madt_entry_interrupt_override_t*)acpi_search_madt(ACPI_MADT_STRUCT_INTERRUPT_OVERRIDE, 10, ((uintptr_t)override) + 1);
    }

    kprintf("IOAPIC: Initialized!\n");
    ioApicInitialized = true;
}
