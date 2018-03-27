#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/acpi/acpi.h>
//#include <kernel/acpi/acpi_tables.h>
#include <acpi.h>


#include <kernel/interrupts/interrupts.h>

static bool acpiInitialized;
/*static acpi_rsdp_t *rsdp;
static acpi_rsdt_t *rsdt;
static acpi_fadt_t *fadt;
static acpi_madt_t *madt;*/

inline bool acpi_supported() {
    return acpiInitialized;
}

/*static acpi_sdt_header_t *acpi_search_rsdt(const char *signature, uint32_t index) {
    // Run through entries and attempt to find the first match after the index specified.
    for (uint32_t entry = index; entry < (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t); entry++) {
        // Map header.
        acpi_sdt_header_t *header = acpi_map_header_temp(rsdt->entries[entry]);

        // Does signature match?
        if (memcmp(header->signature, signature, 4) == 0) {
            // Unmap header and map table.
            acpi_unmap_header_temp();
            uintptr_t page = acpi_map_table(rsdt->entries[entry]);

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
}*/

ACPI_SUBTABLE_HEADER *acpi_search_madt(uint8_t type, uint32_t requiredLength, uintptr_t start) {
    // Get MADT table from ACPI.
    ACPI_TABLE_HEADER *table = NULL;
    ACPI_STATUS status = AcpiGetTable(ACPI_SIG_MADT, 0, &table);
    ACPI_TABLE_MADT *madt = (ACPI_TABLE_MADT*)table;

    // If MADT is null, we cannot search for entries.
    if (status || madt == NULL)
        return NULL;

    kprintf("Found APIC table at 0x%p to 0x%p\n", madt, (uintptr_t)madt + madt->Header.Length);

    // Search for the first matched entry after the start specified.
    uintptr_t address = start == 0 ? (uintptr_t)madt : (uintptr_t)start;
    while (address >= (uintptr_t)madt && address < (uintptr_t)((uintptr_t)madt + madt->Header.Length)) {
        // Get entry.
        ACPI_SUBTABLE_HEADER *header = (ACPI_SUBTABLE_HEADER*)address;
        
        // Ensure entry matches length and type.
        uint8_t length = header->Length;
        if (length == requiredLength && header->Type == type)
            return header;
        else 
            length = 1; // Setting length to 1 will move to the next byte.

        // Move to next entry.
        address += length;
    }

    // No match found.
    kprintf("Nothing found.\n");
    return NULL;
}

void acpi_init() {
    kprintf("ACPI: Initializing...\n");

//AcpiDbgLevel = ACPI_LV_VERBOSITY1 ;


    ACPI_STATUS Status = AcpiInitializeSubsystem();
    Status = AcpiInitializeTables(NULL, 16, false);
    Status = AcpiLoadTables();

    //Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);

    // Get Root System Description Pointer (RSDP).
    /*kprintf("ACPI: Searching for Root System Description Pointer (RSDP)...\n");
    rsdp = acpi_get_rsdp();
    if (rsdp == NULL) {
        kprintf("ACPI: A valid RSDP couldn't be found! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: RSDP table found at 0x%p!\n", rsdp);

    // Print OEM ID and revision.
    char oemId[7];
    strncpy(oemId, rsdp->oemId, 6);
    oemId[6] = '\0';
    kprintf("ACPI:    OEM: \"%s\"\n", oemId);
    kprintf("ACPI:    Revision: %u\n", rsdp->revision);
    kprintf("ACPI:    RSDT: 0x%X\n", rsdp->rsdtAddress);
    kprintf("ACPI:    XSDT: 0x%X\n", rsdp->xsdtAddress);

    // Get Root System Description Table (RSDT).
    kprintf("ACPI: Getting Root System Description Table (RSDT) at 0x%X...\n", rsdp->rsdtAddress);
    rsdt = acpi_get_rsdt(rsdp->rsdtAddress);
    if (rsdt == NULL) {
        kprintf("ACPI: Couldn't get RSDT! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: Mapped RSDT to 0x%p, size: %u bytes\n", rsdt, rsdt->header.length);

    // Get Fixed ACPI Description Table (FADT).
    kprintf("ACPI: Searching for Fixed ACPI Description Table (FADT)...\n");
    fadt = (acpi_fadt_t*)acpi_search_rsdt(ACPI_SIGNATURE_FADT, 0);
    if (fadt == NULL) {
        kprintf("ACPI: Couldn't get FADT! Aborting.\n");
        acpiInitialized = false;
        return;
    }
    kprintf("ACPI: Mapped FADT to 0x%p, size: %u bytes\n", fadt, fadt->header.length);

    // Get Multiple APIC Description Table (MADT).
    kprintf("ACPI: Searching for Multiple APIC Description Table (MADT)...\n");
    madt = (acpi_madt_t*)acpi_search_rsdt(ACPI_SIGNATURE_MADT, 0);
    if (madt != NULL)
        kprintf("ACPI: Mapped MADT to 0x%p, size: %u bytes\n", madt, madt->header.length);
    else
        kprintf("ACPI: No MADT found. APICs and SMP will not be available.\n");*/

    kprintf("ACPI: Initialized!\n");
    acpiInitialized = true;
}

static void acpi_event(ACPI_HANDLE ObjHandle, UINT32 value, void *Context) {
    kprintf_nlock("ACPI: Notify triggered! Value: 0x%X\n", value);

    ACPI_STATUS Status;
ACPI_DEVICE_INFO *Info;
ACPI_BUFFER Path;
char Buffer[256];
Path.Length = sizeof (Buffer);
Path.Pointer = Buffer;

	Status = AcpiGetName(ObjHandle, ACPI_FULL_PATHNAME, &Path);
	Status = AcpiGetObjectInfo(ObjHandle, &Info);

	kprintf_nlock ("%s\n", Path.Pointer);
	kprintf_nlock (" HID: %s, ADR: %.8llX\n", Info->HardwareId.String, Info->Address);
}

static UINT32 acpi_button(void *Context) {
    kprintf_nlock("ACPI: Button fixed triggered\n");
    ACPI_STATUS status = AcpiReset();
    kprintf_nlock("ACPI: Reset status: %d", status);
    outb(0x64, 0xFE);
    return 0;
}

static ACPI_STATUS acpi_pwr_btn_callback(ACPI_HANDLE Object, UINT32                          NestingLevel, void                            *Context, void                            **ReturnValue) {
    kprintf("ACPI: Found power button!\n");
    ACPI_STATUS status = AcpiInstallNotifyHandler(Object, ACPI_DEVICE_NOTIFY, acpi_event, NULL);
    kprintf("ACPI: Status: %d\n", status);
    return AE_OK;
}

void acpi_late_init() {
    ACPI_STATUS status=AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
     kprintf("ACPI: Status: %d\n", status);
	status= AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
     kprintf("ACPI: Status: %d\n", status);
    status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, acpi_event, NULL);
    kprintf("ACPI: Status: %d\n", status);
    //kprintf("ACPI: Status: %d\n", status);
    //d = AcpiInstallGpeHandler(NULL, 0, ACPI_GPE_LEVEL_TRIGGERED, acpi_event, NULL);
   // AcpiEnableGpe(NULL, 0);
    //status=AcpiInstallGlobalEventHandler(acpi_event, NULL);
    //status=AcpiUpdateAllGpes();
    //status = AcpiEnableGpe(NULL, 19);

    status=AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, acpi_button, NULL);
     kprintf("ACPI: Status: %d\n", status);
    status=AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
     kprintf("ACPI: Status: %d\n", status);
    // Find any power button.
    status=AcpiGetDevices("PNP0C0C", acpi_pwr_btn_callback, NULL, NULL);
    kprintf("ACPI: Status: %d\n", status);
    status=AcpiGetDevices("PNP0C0D", acpi_pwr_btn_callback, NULL, NULL);
    kprintf("ACPI: Status: %d\n", status);
    status=AcpiGetDevices("PNP0C0E", acpi_pwr_btn_callback, NULL, NULL);
    kprintf("ACPI: Status: %d\n", status);

   // ACPI_EVENT_STATUS testS = 0;
   // status = AcpiGetGpeStatus(NULL, 19, &testS);
   status = AcpiEnableAllRuntimeGpes();
   kprintf("ACPI: Status RUN GPES: %d\n", status);
   status = AcpiEnableAllWakeupGpes();
    kprintf("ACPI: Status WAK GPES: %d\n", status);
   // kprintf("ACPI: event: 0x%X\n", testS);
uint32_t d = AcpiCurrentGpeCount;
ACPI_EVENT_STATUS sts =0;
status = AcpiUpdateAllGpes();
//status = AcpiGetGpeStatus(NULL, 1, &sts);
  // status = AcpiGetGpeDevice(0, NULL);

    // Get FADT.
    ACPI_TABLE_HEADER *table = NULL;
    status = AcpiGetTable(ACPI_SIG_FADT, 0, &table);
    ACPI_TABLE_FADT *fadt = (ACPI_TABLE_FADT*)table;

    if (!(fadt->Flags & ACPI_FADT_POWER_BUTTON))
        kprintf("ACPI: Fixed power button present.\n");
    if (fadt->Flags & ACPI_FADT_DOCKING_SUPPORTED)
        kprintf("ACPI: Docking supported.\n");
}
