#include <main.h>
#include <kprint.h>
#include <string.h>
#include <arch/i386/kernel/acpi.h>
#include <kernel/paging.h>

// RSDP.
static acpi_rsdp_t *acpi_rsdp;
static acpi_rsdt_t *acpi_rsdt;
static acpi_fadt_t *acpi_fadt;

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

static bool acpi_checksum_sdt(acpi_sdt_header_t *header) {
    // Add bytes of entire table. Checksum dicates the lowest byte must be zero.
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header->length; i++)
        sum += ((uint8_t*)header)[i];
    return sum == 0;
}


static acpi_sdt_header_t *acpi_get_table(uint32_t address, const char *signature) {
    // Get pointer to table.
    acpi_sdt_header_t* table = (acpi_sdt_header_t*)address;

    // Ensure table is valid.
    if (!(memcmp(table->signature, signature, 4) || acpi_checksum_sdt(table)))
        return NULL;

    // Return the table pointer.
    return table;
}

static void acpi_parse_fadt(acpi_fadt_t *fadt) {
    // Save FADT.
    acpi_fadt = fadt;

    // If the SMI port is zero, or the ACPI enable/disable commands are zero, ACPI is already enabled.
    if (fadt->smiCommand == 0 || (fadt->acpiEnable == 0 && fadt->acpiDisable == 0)) {
        kprintf("ACPI: ACPI is already enabled.\n");
        return;
    }

    // Enable ACPI.
    outb(fadt->smiCommand, fadt->acpiEnable);
    kprintf("ACPI: ACPI should now be active.\n");
}

static void acpi_parse_madt(acpi_madt_t* madt) {
    // Print LAPIC address.
    kprintf("ACPI: Local APIC is located at 0x%X.\n", madt->localAddress);

    // Parse entries.
    uint32_t addr = madt->entries;
    while (addr < madt->entries + (madt->header.length - sizeof(acpi_madt_entry_header_t))) {
        // Get entry.
        acpi_madt_entry_header_t *header = (acpi_madt_entry_header_t*)addr;
        
        // If entry is zero bytes in length, abort.
        uint8_t length = header->length;
        if (length == 0) {
            kprintf("ACPI: Zero-length entry found in MADT! Aborting.\n");
            return;
        }

        // Determine type of entry.
        switch (header->type) {
            case ACPI_MADT_STRUCT_LOCAL_APIC: {
                // Get LAPIC entry.
                acpi_madt_entry_local_apic_t *lapic = (acpi_madt_entry_local_apic_t*)header;
                kprintf("ACPI: Found processor %u (%s).\n", lapic->acpiProcessorId, (lapic->flags & ACPI_MADT_ENTRY_LOCAL_APIC_ENABLED) ? "enabled" : "disabled");
                break;
            }

            case ACPI_MADT_STRUCT_IO_APIC: {
                // Get I/O APIC entry.
                acpi_madt_entry_io_apic_t* ioapic = (acpi_madt_entry_io_apic_t*)header;
                kprintf("ACPI: Found I/O APIC %u at 0x%X.\n", ioapic->ioApicId, ioapic->ioApicAddress);
                break;
            }

            case ACPI_MADT_STRUCT_INTERRUPT_OVERRIDE: {
                // Get interrupt source override entry.
                acpi_madt_entry_interrupt_override_t* isro = (acpi_madt_entry_interrupt_override_t*)header;
                kprintf("ACPI: Found Interrupt Source Override.\n");
                kprintf("ACPI:   Bus: %u Source: %u Interrupt: %u Flags: 0x%X\n", isro->bus, isro->source, isro->globalSystemInterrupt, isro->flags);
                break;
            }

            case ACPI_MADT_STRUCT_NMI: {
                kprintf("ACPI: Found Non-maskable Interrupt Source.\n");
                break;
            }

            case ACPI_MADT_STRUCT_LAPIC_NMI: {
                kprintf("ACPI: Found Local APIC NMI.\n");
                break;
            }

            case ACPI_MADT_STRUCT_LAPIC_ADDR_OVERRIDE: {
                kprintf("ACPI: Found Local APIC Address Override.\n");
                break;
            }

            case ACPI_MADT_STRUCT_IO_SAPIC: {
                kprintf("ACPI: Found I/O SAPIC.\n");
                break;
            }

            case ACPI_MADT_STRUCT_LOCAL_SAPIC: {
                kprintf("ACPI: Found Local SAPIC.\n");
                break;
            }

            case ACPI_MADT_STRUCT_PLATFORM_INTERRUPT: {
                kprintf("ACPI: Found Platform Interrupt Sources.\n");
                break;
            }

            case ACPI_MADT_STRUCT_LOCAL_X2APIC: {
                kprintf("ACPI: Found Local x2 APIC.\n");
                break;
            }

            case ACPI_MADT_STRUCT_LOCAL_X2APIC_NMI: {
                kprintf("ACPI: Found Local x2 APIC NMI.\n");
                break;
            }

            case ACPI_MADT_STRUCT_GIC: {
                kprintf("ACPI: Found GIC.\n");
                break;
            }

            case ACPI_MADT_STRUCT_GICD: {
                kprintf("ACPI: Found GICD.\n");
                break;
            }

            default:
                kprintf("ACPI: Unknown MADT entry found at 0x%X.\n", addr);
                length = 1;
                break;
        }

        // Move to next entry.
        addr += length;
    }
}

static bool acpi_parse_table(acpi_sdt_header_t *table) {
    // Checksum table.
    if (!acpi_checksum_sdt(table))
        return false;

    if (memcmp(table->signature, ACPI_SIGNATURE_FADT, 4) == 0) {
        kprintf("ACPI: Found Fixed ACPI Description Table at 0x%X!\n", (uint32_t)table);
        acpi_parse_fadt((acpi_fadt_t*)table);
        return true;
    }
    else if (memcmp(table->signature, ACPI_SIGNATURE_MADT, 4) == 0) {
        kprintf("ACPI: Found Multiple APIC Description Table at 0x%X!\n", (uint32_t)table);
        acpi_parse_madt((acpi_madt_t*)table);
        return true;
    }

    return false;
}

static bool acpi_parse_rsdt(acpi_rsdt_t* rsdt) {
    // Loop through entries.
    for (uint32_t i = 0; i < (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t); i++)
    {
        // Get address of table.
        uint32_t addr = acpi_map_address(rsdt->entries[i]);

        acpi_sdt_header_t* header = (acpi_sdt_header_t*)addr;
        char sig[5];
        strncpy(sig, header->signature, 4);
        sig[4] = '\0';
        kprintf("ACPI: Mapped %s at 0x%X to 0x%X, size: %u bytes\n", sig, rsdt->entries[i], addr, header->length);

        // Parse table.
        acpi_parse_table(header);
    }
}

bool acpi_init() {
    // Get RSDP table.
    acpi_rsdp = acpi_get_rsdp();
    if (acpi_rsdp == NULL) {
        kprintf("A valid RSDP for ACPI couldn't be found! Aborting.\n");
        return false;
    }

    // Print address.
    kprintf("ACPI RSDP table found at 0x%X!\n", (uint32_t)acpi_rsdp);

    // Print OEM ID.
    char oemId[7];
    strncpy(oemId, acpi_rsdp->oemId, 6);
    oemId[6] = '\0';
    kprintf("OEM ID: \"%s\"\n", oemId);
    kprintf("Revision: 0x%X\n", acpi_rsdp->revision);

    // Get RSDT.
    uint32_t addr = acpi_map_address(acpi_rsdp->rsdtAddress);
    acpi_rsdt = (acpi_rsdt_t*)acpi_get_table(addr, ACPI_SIGNATURE_RSDT);
    if (acpi_rsdt == NULL) {
        kprintf("Couldn't get RSDT! Aborting.\n");
        return false;
    }
    kprintf("Mapped RSDT at 0x%X to 0x%X, size: %u bytes\n", acpi_rsdp->rsdtAddress, addr, acpi_rsdt->header.length);

    // Parse RSDT.
    if (acpi_parse_rsdt(acpi_rsdt)) {

    }

    kprintf("ACPI initialized!\n");
    return true;
}
