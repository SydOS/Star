#include <main.h>
#include <kprint.h>
#include <string.h>
#include <math.h>
#include <kernel/acpi/acpi.h>
#include <kernel/acpi/acpi_tables.h>
#include <kernel/memory/paging.h>

acpi_rsdp_t *acpi_get_rsdp() {
    // Search the BIOS area of low memory for the RSDP.
    for (uintptr_t i = 0xC00E0000; i < 0xC00FFFFF; i += 16) {
        uint32_t *block = (uint32_t*)i;
        if ((memcmp(block, ACPI_RSDP_PATTERN1, 4) == 0) && (memcmp(block + 1, ACPI_RSDP_PATTERN2, 4) == 0)) {
            // Verify checksum matches.
            uint32_t checksumResult = 0;
            uint8_t *rsdpBytes = (uint8_t*)block;
            for (uint8_t i = 0; i < 20; i++)
                checksumResult += rsdpBytes[i];

            // If the low byte isn't zero, RSDP is bad.
            if ((uint8_t)checksumResult != 0)
                continue;

            // Return pointer.
            return (acpi_rsdp_t*)block;
        }      
    }

    // RSDP couldn't be discovered.
    return NULL;
}

acpi_sdt_header_t *acpi_map_header_temp(uintptr_t address) {
    // Map header to temp page.
    paging_map_virtual_to_phys(ACPI_TEMP_ADDRESS, address);
    return (acpi_sdt_header_t*)(ACPI_TEMP_ADDRESS + MASK_PAGEFLAGS_4K(address));
}

void acpi_unmap_header_temp() {
    // Unmap the temp page.
    paging_unmap_virtual(ACPI_TEMP_ADDRESS);
}

page_t acpi_map_table(uintptr_t address) {
    // Map header and get number of pages to map.
    acpi_sdt_header_t *header = acpi_map_header_temp(address);
    uint32_t pageCount = DIVIDE_ROUND_UP(MASK_PAGEFLAGS_4K(address) + header->length - 1, PAGE_SIZE_4K);

    // Get next available virtual range.
    page_t page = ACPI_FIRST_ADDRESS;
    uint32_t currentCount = 0;
    uint64_t phys;
    while (currentCount < pageCount) {     
        // Check if page is free.
        if (!paging_get_phys(page, &phys))
            currentCount++;
        else {
            // Move to next page.
            page += PAGE_SIZE_4K;
            if (page > ACPI_LAST_ADDRESS)
                panic("ACPI: Out of virtual addresses!\n");

            // Start back at zero.
            currentCount = 0;
        }
    }

    // Check if last page is outside bounds.
    if (page + ((pageCount - 1) * PAGE_SIZE_4K) > ACPI_LAST_ADDRESS)
        panic("ACPI: Out of virtual addresses!\n");

    // Map range.
    for (uint32_t p = 0; p < pageCount; p++)
        paging_map_virtual_to_phys(page + (p * PAGE_SIZE_4K), MASK_PAGE_4K(address) + (p * PAGE_SIZE_4K));

    // Return address.
    acpi_unmap_header_temp();
    return page + MASK_PAGEFLAGS_4K(address);
}

void acpi_unmap_table(acpi_sdt_header_t *table) {
    // Get page count and unmap table.
    uint32_t pageCount = DIVIDE_ROUND_UP(MASK_PAGEFLAGS_4K((uint32_t)table) + table->length - 1, PAGE_SIZE_4K);
    page_t startPage = MASK_PAGE_4K((uint32_t)table);
    for (page_t page = 0; page < pageCount; page++)
        paging_unmap_virtual(startPage + page);
}

static bool acpi_checksum_sdt(acpi_sdt_header_t *header) {
    kprintf("ACPI: Getting checksum for table 0x%p...", header);

    // Add bytes of entire table. Checksum dicates the lowest byte must be zero.
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header->length; i++)
        sum += ((uint8_t*)header)[i];

    kprintf("%s\n", sum == 0 ? "passed!" : "failed!");
    return sum == 0;
}

acpi_sdt_header_t *acpi_get_table(uintptr_t address, const char *signature) {
    // Get pointer to table.
    acpi_sdt_header_t* table = (acpi_sdt_header_t*)address;
    char sig[5];
    strncpy(sig, table->signature, 4);
    sig[4] = '\0';
    kprintf("ACPI: Getting table %s, address: 0x%p, length: %u\n", sig, address, table->length);

    // Ensure table is valid.
    if (((memcmp(table->signature, signature, 4) != 0) || !acpi_checksum_sdt(table)))
        return NULL;

    // Return the table pointer.
    return table;
}

acpi_rsdt_t *acpi_get_rsdt(uint32_t address) {
    // Map RSDT.
    uint32_t page = acpi_map_table(address);
    return (acpi_rsdt_t*)acpi_get_table(page, ACPI_SIGNATURE_RSDT);
}
