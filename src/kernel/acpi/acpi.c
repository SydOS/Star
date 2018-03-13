#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/acpi/acpi.h>
#include <kernel/acpi/acpi_tables.h>
#include <kernel/paging.h>
#include <arch/i386/kernel/interrupts.h>

static bool acpiInitialized;
static acpi_rsdp_t *rsdp;
static acpi_rsdt_t *rsdt;
static acpi_fadt_t *fadt;
static acpi_madt_t *madt;



inline bool acpi_supported() {
    return acpiInitialized;
}

static acpi_sdt_header_t *acpi_search_rsdt(const char *signature, uint32_t index) {
    // Run through entries and attempt to find the first match after the index specified.
    for (uint32_t entry = index; entry < (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t); entry++) {
        // Map header.
        acpi_sdt_header_t *header = acpi_map_header_temp(rsdt->entries[entry]);

        // Does signature match?
        if (memcmp(header->signature, signature, 4) == 0) {
            // Unmap header and map table.
            acpi_unmap_header_temp();
            page_t page = acpi_map_table(rsdt->entries[entry]);

            // Attempt to get table.
            acpi_sdt_header_t *table = acpi_get_table(page, signature);
            if (table != NULL)
                return table;

            // Table is invalid, unmap it.
            acpi_unmap_table(table);
        }        
    }

    // No match found.
    return NULL;
}

acpi_madt_entry_header_t *acpi_search_madt(uint8_t type, uintptr_t start) {
    // If MADT is null, we cannot search for entries.
    if (madt == NULL)
        return NULL;

    // Search for the first matched entry after the start specified.
    uintptr_t address = start == 0 ? madt->entries : start;
    while (address >= madt->entries && address < (madt->entries + (madt->header.length - sizeof(acpi_madt_entry_header_t)))) {
        // Get entry.
        acpi_madt_entry_header_t *header = (acpi_madt_entry_header_t*)address;
        
        // Ensure entry is not zero bytes.
        uint8_t length = header->length;
        if (length != 0) {
            // Does the type match?
            if (header->type == type)
                return header;
        }
        else {
            // Setting length to 1 will move to the next byte.
            length = 1;
        }

        // Move to next entry.
        address += length;
    }

    // No match found.
    return NULL;
}

void acpi_init() {
    kprintf("ACPI: Initializing...\n");

    // Get Root System Description Pointer (RSDP).
    kprintf("ACPI: Searching for Root System Description Pointer (RSDP)...\n");
    rsdp = acpi_get_rsdp();
    if (rsdp == NULL) {
        kprintf("ACPI: A valid RSDP couldn't be found! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: RSDP table found at 0x%X!\n", (uint32_t)rsdp);

    // Print OEM ID and revision.
    char oemId[7];
    strncpy(oemId, rsdp->oemId, 6);
    oemId[6] = '\0';
    kprintf("ACPI:    OEM: \"%s\"\n", oemId);
    kprintf("ACPI:    Revision: %u\n", rsdp->revision);

    // Get Root System Description Table (RSDT).
    kprintf("ACPI: Getting Root System Description Table (RSDT) at 0x%X...\n", rsdp->rsdtAddress);
    rsdt = acpi_get_rsdt(rsdp->rsdtAddress);
    if (rsdt == NULL) {
        kprintf("ACPI: Couldn't get RSDT! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: Mapped RSDT to 0x%X, size: %u bytes\n", (uint32_t)rsdt, rsdt->header.length);

    // Get Fixed ACPI Description Table (FADT).
    kprintf("ACPI: Searching for Fixed ACPI Description Table (FADT)...\n");
    fadt = (acpi_fadt_t*)acpi_search_rsdt(ACPI_SIGNATURE_FADT, 0);
    if (fadt == NULL) {
        kprintf("ACPI: Couldn't get FADT! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: Mapped FADT to 0x%X, size: %u bytes\n", (uint32_t)fadt, fadt->header.length);

    // Get Multiple APIC Description Table (MADT).
    kprintf("ACPI: Searching for Multiple APIC Description Table (MADT)...\n");
    madt = (acpi_madt_t*)acpi_search_rsdt(ACPI_SIGNATURE_MADT, 0);
    if (madt != NULL)
        kprintf("ACPI: Mapped MADT to 0x%X, size: %u bytes\n", (uint32_t)madt, madt->header.length);
    else
        kprintf("ACPI: No MADT found. APICs and SMP will not be available.\n");

    // Find IOAPIC.
    acpi_madt_entry_io_apic_t *apic = (acpi_madt_entry_io_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_IO_APIC, 0);

    kprintf("ACPI: Initialized!\n");
    acpiInitialized = true;
}
