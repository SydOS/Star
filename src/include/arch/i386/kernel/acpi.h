#ifndef ACPI_H
#define ACPI_H

#include <main.h>

#define ACPI_RSDT_ADDRESS   0xFF000000

#define ACPI_RSDP_PATTERN1  0x20445352 // "RSD "
#define ACPI_RSDP_PATTERN2  0x20525450 // "PTR "
#define ACPI_RSDT_SIGNATURE "RSDT" // "RSDT"

// Root System Description Pointer.
struct acpi_rsdp {
    uint64_t signature;         // "RSD PTR " (Notice that this signature must contain a trailing blank character.)
    uint8_t  checksum;          // This is the checksum of the fields defined in the ACPI 1.0 specification. This includes
                                // only the first 20 bytes of this table, bytes 0 to 19, including the checksum field. These bytes must sum to zero.
    char     oemId[6];          // An OEM-supplied string that identifies the OEM.
    uint8_t  revision;          // The revision of this structure. Larger revision numbers are backward compatible to lower revision numbers.
    uint32_t rsdtAddress;       // 32 bit physical address of the RSDT.

    uint32_t length;            // The length of the table, in bytes, including the header, starting from offset 0.
                                // This field is used to record the size of the entire table.
    uint64_t xsdtAddress;       // 64 bit physical address of the XSDT.
    uint8_t  checksumExtended;  // This is a checksum of the entire table, including both checksum fields.
    uint8_t  reserved[3];       // Reserved fields.
} __attribute__((packed));
typedef struct acpi_rsdp acpi_rsdp_t;

// System Description Table header.
struct acpi_sdt_header {
    char     signature[4];      // Signature for the table.
    uint32_t length;            // Length, in bytes, of the entire table. The length implies the number of Entry fields (n) at the end of the table.
    uint8_t  revision;          // 1.
    uint8_t  checksum;          // Entire table must sum to zero.
    char     oemId[6];          // OEM ID.
    uint64_t oemTableId;        // For the table, the table ID is the manufacture model ID. This field must match the OEM Table ID in the FADT.
    uint32_t oemRevision;       // OEM revision of the table for supplied OEM Table ID.
    uint32_t creatorId;         // Vendor ID of utility that created the table. For tables containing Definition Blocks, this is the ID for the ASL Compiler.
    uint32_t creatorRevision;   // Revision of utility that created the table. For tables containing Definition Blocks, this is the revision for the ASL Compiler.
} __attribute__((packed));
typedef struct acpi_sdt_header acpi_sdt_header_t;

// Root System Description Table.
struct acpi_rsdt {
    acpi_sdt_header_t header;   // Header.
    uint32_t entries[0];        // Array of pointers to other ACPI tables.
} __attribute__((packed));
typedef struct acpi_rsdt acpi_rsdt_t;

extern void acpi_init();

#endif