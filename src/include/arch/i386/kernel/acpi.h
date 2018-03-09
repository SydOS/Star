#ifndef ACPI_H
#define ACPI_H

#include <main.h>

#define ACPI_RSDT_ADDRESS   0xFF000000

#define ACPI_RSDP_PATTERN1  0x20445352 // "RSD "
#define ACPI_RSDP_PATTERN2  0x20525450 // "PTR "

#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_BERT_SIGNATURE "BERT"
#define ACPI_BGRT_SIGNATURE "BGRT"
#define ACPI_CPEP_SIGNATURE "CPEP"
#define ACPI_DSDT_SIGNATURE "DSDT"
#define ACPI_ECDT_SIGNATURE "ECDT"
#define ACPI_EINJ_SIGNATURE "EINJ"
#define ACPI_ERST_SIGNATURE "ERST"
#define ACPI_FADT_SIGNATURE "FACP"
#define ACPI_FACS_SIGNATURE "FACS"
#define ACPI_FPDT_SIGNATURE "FPDT"
#define ACPI_GTDT_SIGNATURE "GTDT"
#define ACPI_HEST_SIGNATURE "HEST"
#define ACPI_MSCT_SIGNATURE "MSCT"
#define ACPI_MPST_SIGNATURE "MPST"
#define ACPI_PMTT_SIGNATURE "PMTT"
#define ACPI_PSDT_SIGNATURE "PSDT"
#define ACPI_RASF_SIGNATURE "RASF"
#define ACPI_RSDT_SIGNATURE "RSDT"
#define ACPI_SBST_SIGNATURE "SBST"
#define ACPI_SLIT_SIGNATURE "SLIT"
#define ACPI_SRAT_SIGNATURE "SRAT"
#define ACPI_SSDT_SIGNATURE "SSDT"
#define ACPI_XSDT_SIGNATURE "XSDT"

#define ACPI_BOOT_SIGNATURE "BOOT"
#define ACPI_CSRT_SIGNATURE "CSRT"
#define ACPI_DBG2_SIGNATURE "DBG2"
#define ACPI_DBGP_SIGNATURE "DBGP"
#define ACPI_DMAR_SIGNATURE "DMAR"
#define ACPI_DRTM_SIGNATURE "DRTM"
#define ACPI_ETDT_SIGNATURE "ETDT"
#define ACPI_HPET_SIGNATURE "HPET"
#define ACPI_IBFT_SIGNATURE "IBFT"
#define ACPI_IVRS_SIGNATURE "IVRS"
#define ACPI_MCFG_SIGNATURE "MCFG"
#define ACPI_MCHI_SIGNATURE "MCHI"
#define ACPI_MSDM_SIGNATURE "MSDM"
#define ACPI_SLIC_SIGNATURE "SLIC"
#define ACPI_SPCR_SIGNATURE "SPCR"
#define ACPI_SPMI_SIGNATURE "SPMI"
#define ACPI_TCPA_SIGNATURE "TCPA"
#define ACPI_TPM2_SIGNATURE "TPM2"
#define ACPI_UEFI_SIGNATURE "UEFI"
#define ACPI_WAET_SIGNATURE "WAET"
#define ACPI_WDAT_SIGNATURE "WDAT"
#define ACPI_WDRT_SIGNATURE "WDRT"
#define ACPI_WPBT_SIGNATURE "WPBT"

enum acpi_power_profiles {
    ACPI_POWER_PROFILE_UNSPECIFIED  = 0,
    ACPI_POWER_PROFILE_DESKTOP      = 1,
    ACPI_POWER_PROFILE_MOBILE       = 2,
    ACPI_POWER_PROFILE_WORKSTATION  = 3,
    ACPI_POWER_PROFILE_ENT_SERVER   = 4,
    ACPI_POWER_PROFILE_SOHO_SERVER  = 5,
    ACPI_POWER_PROFILE_APPLIANCE    = 6,
    ACPI_POWER_PROFILE_PERF_SERVER  = 7,
    ACPI_POWER_PROFILE_TABLET       = 8
};

enum acpi_feature_flags {
    // Processor properly implements a functional equivalent to the WBINVD IA-32 instruction.
    ACPI_FEATURE_WBINVD                     = 0x000001,

    // If set, indicates that the hardware flushes all caches on the WBINVD instruction and maintains memory coherency.
    ACPI_FEATURE_WBINVD_FLUSH               = 0x000002,

    // A one indicates that the C1 power state is supported on all processors.
    ACPI_FEATURE_PROC_C1                    = 0x000004,

    // A zero indicates that the C2 power state is configured to only work on a uniprocessor (UP) system.
    // A one indicates that the C2 power state is configured to work on a UP or multiprocessor (MP) system.
    ACPI_FEATURE_P_LVL2_UP                  = 0x000008,

    // A zero indicates the power button is handled as a fixed feature programming model; a one indicates
    // the power button is handled as a control method device.
    ACPI_FEATURE_PWR_BUTTON                 = 0x000010,

    // A zero indicates the sleep button is handled as a fixed feature programming model; a one indicates
    // the sleep button is handled as a control method device.
    ACPI_FEATURE_SLP_BUTTON                 = 0x000020,

    // A zero indicates the RTC wake status is supported in fixed register space; a one indicates the RTC wake
    // status is not supported in fixed register space.
    ACPI_FEATURE_FIX_RTC                    = 0x000040,

    // Indicates whether the RTC alarm function can wake the system from the S4 state.
    ACPI_FEATURE_RTC_S4                     = 0x000080,

    // A zero indicates TMR_VAL is implemented as a 24-bit value. A one indicates TMR_VAL is implemented as a 32-bit value.
    ACPI_FEATURE_TMR_VAL_EXT                = 0x000100,

    // A zero indicates that the system cannot support docking. A one indicates that the system can support docking.
    // Notice that this flag does not indicate whether or not a docking station is currently present; it only indicates
    // that the system is capable of docking.
    ACPI_FEATURE_DCK_CAP                    = 0x000200,

    // If set, indicates the system supports system reset via the FADT RESET_REG.
    ACPI_FEATURE_RESET_REG_SUP              = 0x000400,

    // System Type Attribute. If set indicates that the system has no internal expansion capabilities and the case is sealed.
    ACPI_FEATURE_SEALED_CASE                = 0x000800,

    // System Type Attribute. If set indicates the system cannot detect the monitor or keyboard / mouse devices.
    ACPI_FEATURE_HEADLESS                   = 0x001000,

    // If set, indicates to OSPM that a processor native instruction must be executed after writing the SLP_TYPx register.
    ACPI_FEATURE_CPU_SW_SLP                 = 0x002000,

    // set, indicates the platform supports the PCIEXP_WAKE_STS bit in the PM1 Status register and the PCIEXP_WAKE_EN bit
    // in the PM1 Enable register. This bit must be set on platforms containing chipsets that implement PCI Express.
    ACPI_FEATURE_PCI_EXP_WAK                = 0x004000,
    
    // A value of one indicates that OSPM should use a platform provided timer to drive any monotonically non-decreasing
    // counters, such as OSPM performance counter services.
    ACPI_FEATURE_USE_PLATFORM_CLOCK         = 0x008000,

    //A one indicates that the contents of the RTC_STS flag is valid when waking the system from S4.
    ACPI_FEATURE_S4_RTC_STS_VALID           = 0x010000,

    // A one indicates that the platform is compatible with remote power-on.
    ACPI_FEATURE_REMOTE_POWER_ON_CAPABLE    = 0x020000,

    // A one indicates that all local APICs must be configured for the cluster destination model when delivering interrupts
    // in logical mode.
    ACPI_FEATURE_FORCE_APIC_CLUSTER_MODEL   = 0x040000,

    // A one indicates that all local xAPICs must be configured for physical destination mode.
    ACPI_FEATURE_FORCE_APIC_PHYSICAL_DESTINATION_MODE = 0x080000,

    // A one indicates that the ACPI Hardware Interface is not implemented.
    ACPI_FEATURE_HW_REDUCED_ACPI            = 0x100000,

    // A one informs OSPM that the platform is able to achieve power savings in S0 similar to or better
    // than those typically achieved in S3.
    ACPI_FEATURE_LOW_POWER_S0_IDLE_CAPABLE  = 0x200000
};

struct acpi_generic_address {
    uint8_t addressSpace;
    uint8_t bitWidth;
    uint8_t bitOffset;
    uint8_t accessSize;
    uint64_t address;
} __attribute__((packed));
typedef struct acpi_generic_address acpi_generic_address_t;

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

// Root System Description Table (RSDT).
struct acpi_rsdt {
    acpi_sdt_header_t header;   // Header.
    uint32_t entries[0];        // Array of pointers to other ACPI tables.
} __attribute__((packed));
typedef struct acpi_rsdt acpi_rsdt_t;

// eXtended System Description Table (XSDT).
struct acpi_xsdt {
    acpi_sdt_header_t header;   // Header.
    uint64_t entries[0];        // Array of pointers to other ACPI tables.
} __attribute__((packed));
typedef struct acpi_xsdt acpi_xsdt_t;

// Fixed ACPI Description Table (FADT).
struct acpi_fadt {
    acpi_sdt_header_t header;   // Header.
    uint32_t firmwareCtrl;      // Physical memory address of the FACS.
    uint32_t dsdt;              // Physical memory address (0-4 GB) of the DSDT.
    uint8_t reserved1;          // ACPI 1.0 defined this offset as a field named INT_MODEL, which was eliminated in ACPI 2.0. Platforms should set this
                                //  field to zero but field values of one are also allowed to maintain compatibility with ACPI 1.0.

    uint8_t preferredPmProfile; // This field is set by the OEM to convey the preferred power management profile to OSPM.
    uint16_t sciInterrupt;      // System vector the SCI interrupt is wired to in 8259 mode.
    uint32_t smiCommand;        // System port address of the SMI Command Port.
    uint8_t acpiEnable;         // The value to write to SMI_CMD to disable SMI ownership of the ACPI hardware registers.
    uint8_t acpiDisable;        // The value to write to SMI_CMD to re-enable SMI ownership of the ACPI hardware registers.
    uint8_t s4BiosReq;          // The value to write to SMI_CMD to enter the S4BIOS state.
    uint8_t pStateCnt;          // If non-zero, this field contains the value OSPM writes to the SMI_CMD register to assume
                                //  processor performance state control responsibility.

    uint32_t pm1aEvtBlk;        // System port address of the PM1a Event Register Block.
    uint32_t pm1bEvtBlk;        // System port address of the PM1b Event Register Block.
    uint32_t pm1aCntBlk;        // System port address of the PM1a Control Register Block.
    uint32_t pm1bCntBlk;        // System port address of the PM1b Control Register Block.
    uint32_t pm2CntBlk;         // System port address of the PM2 Control Register Block.
    uint32_t pmTmrBlk;          // System port address of the Power Management Timer Control Register Block.
    uint32_t gpe0Blk;           // System port address of General-Purpose Event 0 Register Block.
    uint32_t gpe1Blk;           // System port address of General-Purpose Event 1 Register Block.
    uint8_t pm1EvtLen;          // Number of bytes decoded by PM1a_EVT_BLK and, if supported, PM1b_EVT_BLK.
    uint8_t pm1CntLen;          // Number of bytes decoded by PM1a_CNT_BLK and, if supported, PM1b_CNT_BLK.
    uint8_t pm2CntLen;          // Number of bytes decoded by PM2_CNT_BLK.
    uint8_t pmTmrLen;           // Number of bytes decoded by PM_TMR_BLK.
    uint8_t gpe0BlkLen;         // Number of bytes decoded by GPE0_BLK.
    uint8_t gpe1BlkLen;         // Number of bytes decoded by GPE1_BLK.
    uint8_t gpe1Base;           // Offset within the ACPI general-purpose event model where GPE1 based events start.
    uint8_t cstCnt;             // If non-zero, this field contains the value OSPM writes to the SMI_CMD register to indicate OS support
                                //  for the _CST object and C States Changed notification.

    uint16_t pLvl2Lat;          // The worst-case hardware latency, in microseconds, to enter and exit a C2 state.
    uint16_t pLvl3Lat;          // The worst-case hardware latency, in microseconds, to enter and exit a C3 state.
    uint16_t flushSize;         // If WBINVD=0, the value of this field is the number of flush strides that need to be read
                                //  (using cacheable addresses) to completely flush dirty lines from any processor’s memory caches.
    uint16_t flushStride;       // If WBINVD=0, the value of this field is the cache line width, in bytes, of the processor’s memory caches.

    uint8_t dutyOffset;         // The zero-based index of where the processor’s duty cycle setting is within the processor’s P_CNT register.
    uint8_t dutyWidth;          // The bit width of the processor’s duty cycle setting value in the P_CNT register.

    uint8_t dayAlarm;           // The RTC CMOS RAM index to the day-of-month alarm value.
    uint8_t monAlarm;           // The RTC CMOS RAM index to the month of year alarm value.
    uint8_t century;            // The RTC CMOS RAM index to the century of data value (hundred and thousand year decimals).

    uint16_t iapcBootArch;      // IA-PC Boot Architecture Flags.
    uint8_t reserved2;          // Must be 0.

    uint32_t flags;             // Fixed feature flags.

    acpi_generic_address_t resetReg;        // The address of the reset register represented in Generic Address Structure format.
    uint8_t resetValue;                     // Indicates the value to write to the RESET_REG port to reset the system.
    uint8_t reserved3[3];                   // Must be 0.

    uint64_t xFirmwareCtrl;                 // 64bit physical address of the FACS.
    uint64_t xDsdt;                         // 64bit physical address of the DSDT.
    acpi_generic_address_t xPm1aEvtBlk;     // Extended address of the PM1a Event Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xPm1bEvtBlk;     // Extended address of the PM1b Event Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xPm1aCntBlk;     // Extended address of the PM1a Control Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xPm1bCntBlk;     // Extended address of the PM1b Control Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xPm2CntBlk;      // Extended address of the Power Management 2 Control Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xPmTmrBlk;       // Extended address of the Power Management Timer Control Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xGpe0Blk;        // Extended address of the General-Purpose Event 0 Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t xGpe1Blk;        // Extended address of the General-Purpose Event 1 Register Block, represented in Generic Address Structure format.
    acpi_generic_address_t sleepControlReg; // The address of the Sleep register, represented in Generic Address Structure format.
    acpi_generic_address_t sleepStatusReg;  // The address of the Sleep status register, represented in Generic Address Structure format.
} __attribute__((packed));
typedef struct acpi_fadt acpi_fadt_t;

extern void acpi_init();

#endif