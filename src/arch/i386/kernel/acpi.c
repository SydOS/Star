#include <main.h>
#include <kprint.h>
#include <string.h>
#include <arch/i386/kernel/acpi.h>
#include <kernel/paging.h>

// RSDP.
static acpi_rsdp_t *rsdp;
static acpi_rsdt_t *rsdt;

static acpi_rsdp_t *acpi_get_rsdp() {
    for (uint32_t i = 0xC00E0000; i < 0xC00FFFFF; i += 16) {
        uint32_t *block = (uint32_t*)i;
        if (block[0] == ACPI_RSDP_PATTERN1 && block[1] == ACPI_RSDP_PATTERN2) {
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
    return NULL;
}

static bool acpi_checksum_sdt(acpi_sdt_header_t header) {
    // Add bytes of entire table. Checksum dicates the lowest byte must be zero.
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header.length; i++)
        sum += ((uint8_t*)&header)[i];
    return sum == 0;
}

static acpi_rsdt_t *acpi_get_rsdt(uint32_t rsdtAddress) {
    // Map RSDT to virtual memory.
    paging_map_virtual_to_phys(ACPI_RSDT_ADDRESS, rsdtAddress);
    uint32_t add = ACPI_RSDT_ADDRESS + MASK_PAGEFLAGS_4K(rsdtAddress);
    // Get pointer to RSDT.
    acpi_rsdt_t* rsdt = (acpi_rsdt_t*)add;

    // Ensure RSDT is valid.
    if (rsdt->header.signature != ACPI_RSDT_SIGNATURE && acpi_checksum_sdt(rsdt->header))
        return NULL;

    // Return the RSDT pointer.
    return rsdt;
}

void acpi_init() {
    // Get RSDP table.
    rsdp = acpi_get_rsdp();
    if (rsdp == NULL)
        return;

    // Print address.
    kprintf("ACPI RSDP table found at 0x%X!\n", (uint32_t)rsdp);

    // Print OEM ID.
    char oemId[7];
    strncpy(oemId, rsdp->oemId, 6);
    oemId[6] = '\0';
    kprintf("OEM ID: \"%s\"\n", oemId);
    kprintf("Revision: 0x%X\n", rsdp->revision);

    // Get RSDT.
    rsdt = acpi_get_rsdt(rsdp->rsdtAddress);
    if (rsdt == NULL)
        return;

    kprintf("Got RSDT at 0x%X, size: %u bytes\n", rsdp->rsdtAddress, rsdt->header.length); 
}