#ifndef ACPI_H
#define ACPI_H

#include <main.h>
#include <kernel/acpi/acpi_tables.h>

#define ACPI_TEMP_ADDRESS   0xFEF00000
#define ACPI_FIRST_ADDRESS  0xFEF01000
#define ACPI_LAST_ADDRESS   0xFEFFF000

extern inline bool acpi_supported();
extern acpi_madt_entry_header_t *acpi_search_madt(uint8_t type, uintptr_t start);
extern void acpi_init();

#endif
