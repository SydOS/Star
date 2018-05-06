#ifndef ACPI_H
#define ACPI_H

#include <main.h>
#include <acpi.h>
//#include <kernel/acpi/acpi_tables.h>

#ifdef X86_64
#define ACPI_TEMP_ADDRESS   0xFFFFFF00FEF00000
#define ACPI_FIRST_ADDRESS  0xFFFFFF00FEF01000
#define ACPI_LAST_ADDRESS   0xFFFFFF00FEFFF000
#else
#define ACPI_TEMP_ADDRESS   0xFEF00000
#define ACPI_FIRST_ADDRESS  0xFEF01000
#define ACPI_LAST_ADDRESS   0xFEFFF000
#endif

extern bool acpi_supported();
extern ACPI_SUBTABLE_HEADER *acpi_search_madt(uint8_t type, uint32_t requiredLength, uintptr_t start);
extern bool acpi_change_pic_mode(uint32_t value);
extern void acpi_init();

#endif
