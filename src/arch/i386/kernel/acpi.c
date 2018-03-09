#include <main.h>
#include <kprint.h>
#include <string.h>
#include <arch/i386/kernel/acpi.h>
#include <kernel/paging.h>

// RSDP.
static acpi_rsdp_t *rsdp;
static acpi_rsdt_t *rsdt;
static acpi_fadt_t *fadt;

static acpi_rsdp_t *acpi_get_rsdp() {
    // Search the BIOS area of low memory for the RSDP.
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

    // RSDP couldn't be discovered.
    return NULL;
}

static uint32_t acpi_map_address(uint32_t address) {
    // Is the address base already mapped?
    uint32_t base = MASK_PAGE_4K(address);
    for (uint32_t page = ACPI_FIRST_ADDRESS; page <= ACPI_LAST_ADDRESS; page += PAGE_SIZE_4K) {
        // Get physical mapping for virtual address.
        uint64_t physicalAddress = 0;
        if (paging_get_phys(page, &physicalAddress)) {
            // Does the address passed fit into the same 4KB space as the
            // physical address belonging to the current virtual one?
            if ((uint32_t)physicalAddress == base)
                return page + MASK_PAGEFLAGS_4K(address);
        }
        else {
            // Map address block to virtual memory.
            paging_map_virtual_to_phys(page, base);
            return page + MASK_PAGEFLAGS_4K(address);
        }
    }

    // No address could be found!
    panic("ACPI out of virtual addresses!\n");
}

static bool acpi_checksum_sdt(acpi_sdt_header_t header) {
    // Add bytes of entire table. Checksum dicates the lowest byte must be zero.
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header.length; i++)
        sum += ((uint8_t*)&header)[i];
    return sum == 0;
}

static acpi_sdt_header_t *acpi_get_table(uint32_t address, const char *signature) {
    // Get pointer to table.
    acpi_sdt_header_t* table = (acpi_sdt_header_t*)address;

    // Ensure table is valid.
    if (memcmp(table->signature, signature, 4) || acpi_checksum_sdt(*table))
        return NULL;

    // Return the table pointer.
    return table;
}

static uint8_t acpi_get_madt_entry(uint8_t *entry) {
    // Get type.
    uint8_t type = entry[0];
    uint8_t length = entry[1];

    // Determine type.
    switch (type) {
        case ACPI_MADT_STRUCT_LOCAL_APIC:
            if (length == 8)
                return length;

        case ACPI_MADT_STRUCT_IO_APIC:
            if (length == 12)
                return length;

        case ACPI_MADT_STRUCT_INTERRUPT_OVERRIDE:
            if (length == 10)
                return length;

        case ACPI_MADT_STRUCT_NMI:
            if (length == 8)
                return length;

        case ACPI_MADT_STRUCT_LAPIC_NMI:
            if (length == 6)
                return length;

        case ACPI_MADT_STRUCT_LAPIC_ADDR_OVERRIDE:
            if (length == 12)
                return length;

        case ACPI_MADT_STRUCT_IO_SAPIC:
            if (length == 16)
                return length;

        case ACPI_MADT_STRUCT_PLATFORM_INTERRUPT:
            if (length == 16)
                return length;

        case ACPI_MADT_STRUCT_LOCAL_X2APIC:
            if (length == 16)
                return length;

        case ACPI_MADT_STRUCT_LOCAL_X2APIC_NMI:
            if (length == 12)
                return length;

        case ACPI_MADT_STRUCT_GIC:
            if (length == 40)
                return length;

        case ACPI_MADT_STRUCT_GICD:
            if (length == 24)
                return length;
    }

    return 0;
}

bool acpi_init() {
    // Get RSDP table.
    rsdp = acpi_get_rsdp();
    if (rsdp == NULL) {
        kprintf("A valid RSDP for ACPI couldn't be found! Aborting.\n");
        return false;
    }

    // Print address.
    kprintf("ACPI RSDP table found at 0x%X!\n", (uint32_t)rsdp);

    // Print OEM ID.
    char oemId[7];
    strncpy(oemId, rsdp->oemId, 6);
    oemId[6] = '\0';
    kprintf("OEM ID: \"%s\"\n", oemId);
    kprintf("Revision: 0x%X\n", rsdp->revision);

    // Get RSDT.
    uint32_t addr = acpi_map_address(rsdp->rsdtAddress);
    rsdt = (acpi_rsdt_t*)acpi_get_table(addr, ACPI_RSDT_SIGNATURE);
    if (rsdt == NULL) {
        kprintf("Couldn't get RSDT! Aborting.\n");
        return false;
    }
    kprintf("Mapped RSDT at 0x%X to 0x%X, size: %u bytes\n", rsdp->rsdtAddress, addr, rsdt->header.length);

    // Loop through entries.
    for (uint32_t i = 0; i < (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t); i++)
    {
        // Get address of table.
        addr = acpi_map_address(rsdt->entries[i]);

        acpi_sdt_header_t* header = (acpi_sdt_header_t*)addr;
        char sig[5];
        strncpy(sig, header->signature, 4);
        sig[4] = '\0';
        kprintf("Mapped %s at 0x%X to 0x%X, size: %u bytes\n", sig, rsdt->entries[i], addr, header->length);

        // Is it a FADT?
        if (memcmp(header->signature, ACPI_FADT_SIGNATURE, 4) == 0)
        {
            fadt = (acpi_fadt_t*)acpi_get_table(addr, ACPI_FADT_SIGNATURE);
            kprintf("Got FADT at 0x%X, size: %u bytes\n", addr, fadt->header.length);
        }
        else if (memcmp(header->signature, ACPI_MADT_SIGNATURE, 4) == 0)
        {
            acpi_madt_t *madt = (acpi_madt_t*)acpi_get_table(addr, ACPI_MADT_SIGNATURE);
            kprintf("Got MADT at 0x%X, size: %u bytes\n", addr, madt->header.length);

            // Iterate through MADT strucutres.
            uint32_t a = 0;
            while (a < (madt->header.length - sizeof(acpi_madt_entry_header_t)) / sizeof(uint8_t)) {
                if (acpi_get_madt_entry((madt->entries)+a) != 0) {
                    // Determine type.
                    //switch (entries[a+1]) {
                    //    case ACPI_MADT_STRUCT_LOCAL_APIC:
                            
                  //          break;
                 //   }
                    kprintf("Got MADT entry %u at 0x%X, length: %u bytes\n", madt->entries[a], madt->entries+a, madt->entries[a+1]);
                    a += madt->entries[a+1];
                }
                else {
                    a++;
                }
            }
        }
    }

    kprintf("ACPI initialized!\n");
    return true;
}