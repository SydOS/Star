#include <main.h>
#include <kprint.h>
#include <string.h>
#include <kernel/acpi/acpi.h>
//#include <kernel/acpi/acpi_tables.h>
#include <acpi.h>


#include <kernel/interrupts/interrupts.h>

static bool acpiInitialized;

inline bool acpi_supported() {
    return acpiInitialized;
}

ACPI_SUBTABLE_HEADER *acpi_search_madt(uint8_t type, uint32_t requiredLength, uintptr_t start) {
    // Get MADT table from ACPI.
    ACPI_TABLE_HEADER *table = NULL;
    ACPI_STATUS status = AcpiGetTable(ACPI_SIG_MADT, 0, &table);
    ACPI_TABLE_MADT *madt = (ACPI_TABLE_MADT*)table;

    // If MADT is null, we cannot search for entries.
    if (status || madt == NULL)
        return NULL;

    // Search for the first matched entry after the start specified.
    uintptr_t address = start == 0 ? ((uintptr_t)madt + sizeof(ACPI_TABLE_MADT)) : (uintptr_t)start;
    while (address >= ((uintptr_t)madt + sizeof(ACPI_TABLE_MADT)) && address < (uintptr_t)((uintptr_t)madt + madt->Header.Length)) {
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
    return NULL;
}

static ACPI_STATUS acpi_get_prt_method_callback(ACPI_HANDLE object, UINT32 nestingLevel, void *context, void **returnValue) {
    ACPI_STATUS status = AE_OK;
    ACPI_HANDLE *handle = (ACPI_HANDLE)context;

    // Get parent.
    ACPI_HANDLE parentHandle;
    status = AcpiGetParent(object, &parentHandle);
    if (status) {
        *returnValue = false;
        return status;
    }

    // Get name.
    ACPI_BUFFER path = { ACPI_ALLOCATE_BUFFER };
    status = AcpiGetName(object, ACPI_SINGLE_NAME, &path);
    if (status) {
        if (path.Pointer)
            ACPI_FREE(path.Pointer);
        *returnValue = false;
        return status;
    }

    // Verify name is _PRT.
    if (strcmp((char*)path.Pointer, "_PRT") == 0 && *handle == parentHandle) {
        *returnValue = true;
        status = AE_CTRL_TERMINATE;
    }

    // Free name object and return status.
    if (path.Pointer)
        ACPI_FREE(path.Pointer);
    return status;
}

static ACPI_STATUS acpi_get_prt_device_callback(ACPI_HANDLE object, UINT32 nestingLevel, void *context, void **returnValue) {
    // Terminate the walk when done.
    ACPI_STATUS status = AE_OK;
    uint32_t *busAddress = (uint32_t*)context;

    // Get device info.
    ACPI_DEVICE_INFO *info;
	status = AcpiGetObjectInfo(object, &info);
    UINT64 address = 0;
    if (status == AE_OK)
        address = info->Address;

    // Free device object.
    if (info)
        ACPI_FREE(info);

    // Ensure command completed.
    if (status == AE_OK) {
        // Ensure device has a _PRT method.
        bool hasPrt = false;
        //kprintf("ACPI: Checking _PRT on device ADR 0x%X...\n", address);
        status = AcpiWalkNamespace(ACPI_TYPE_METHOD, object, 1, acpi_get_prt_method_callback, NULL, &object, &hasPrt);

        // Is the device the one we want?
        if (status == AE_OK && hasPrt && (busAddress == NULL || address == *busAddress)) {
            kprintf("ACPI: Found our PCI device: 0x%llX\n", address);
            status = AE_CTRL_TERMINATE;
            *returnValue = object;
        }
    }

    // Return status.
    return status;
}

ACPI_STATUS acpi_get_prt(uint32_t busAddress, ACPI_BUFFER *outBuffer) {
    // Get handle to root PCI bus.
    ACPI_HANDLE rootHandle = 0;
    kprintf("ACPI: Finding root PCI bus device...\n");
    ACPI_STATUS status = AcpiGetDevices("PNP0A03", acpi_get_prt_device_callback, NULL, &rootHandle);
    if (status)
        return status;
    if (rootHandle == 0)
        return AE_ERROR;

    // Get handle to PCI bus.
    ACPI_HANDLE handle = 0;
    if (busAddress != 0) {
        kprintf("ACPI: Finding PCI bus device 0x%X...\n", busAddress);
        status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, rootHandle, -1, acpi_get_prt_device_callback, NULL, &busAddress, &handle);
        if (status)
            return status;
        if (handle == 0)
            return AE_ERROR;
    }
    else {
        // We already have the root bus handle.
        handle = rootHandle;
    }

    // Get name of device for debug.
    ACPI_BUFFER path = { ACPI_ALLOCATE_BUFFER };
    status = AcpiGetName(handle, ACPI_FULL_PATHNAME, &path);
    if (!status)
        kprintf("ACPI: Got PCI bus %s!\n", (char*)path.Pointer);
    if (path.Pointer)
        ACPI_FREE(path.Pointer);

    // Get routing table.
    kprintf("ACPI: Getting IRQ routing table...\n");
    ACPI_BUFFER buffer = { ACPI_ALLOCATE_BUFFER };
    status = AcpiGetIrqRoutingTable(handle, &buffer);

    // Return buffer and status.
    *outBuffer = buffer;
    return status;
}

bool acpi_change_pic_mode(uint32_t value) {
    kprintf("ACPI: Executing _PIC method with parameter 0x%X...\n", value);

    // Build param list.
	ACPI_OBJECT_LIST list = {};
	ACPI_OBJECT obj = {};
	obj.Integer.Value = value;
	obj.Type = ACPI_TYPE_INTEGER;
    list.Count = 1;
	list.Pointer = &obj;

    // Execute method.
	return AcpiEvaluateObject(NULL, "\\_PIC", &list, NULL) == AE_OK;
}

void acpi_init() {
    kprintf("ACPI: Initializing...\n");

    ACPI_STATUS Status = AcpiInitializeSubsystem();
    kprintf("acpi status: 0x%X\n", Status);
    Status = AcpiInitializeTables(NULL, 16, false);
    kprintf("acpi status: 0x%X\n", Status);
    Status = AcpiLoadTables();
    kprintf("acpi status: 0x%X\n", Status);
    kprintf("ACPI: Initialized!\n");
    acpiInitialized = true;
}

static void acpi_event(ACPI_HANDLE ObjHandle, UINT32 value, void *Context) {
    kprintf("ACPI: Notify triggered! Value: 0x%X\n", value);

    ACPI_STATUS Status;
ACPI_DEVICE_INFO *Info;
ACPI_BUFFER Path;
char Buffer[256];
Path.Length = sizeof (Buffer);
Path.Pointer = Buffer;

	Status = AcpiGetName(ObjHandle, ACPI_FULL_PATHNAME, &Path);
	Status = AcpiGetObjectInfo(ObjHandle, &Info);

	kprintf ("%s\n", Path.Pointer);
	kprintf(" HID: %s, ADR: %.8llX\n", Info->HardwareId.String, Info->Address);

    if (strcmp(Info->HardwareId.String, "PNP0C0C") == 0 && value == 0x80) {
        kprintf("ACPI: Power button pressed, entering S5!\n");
        sleep(1000);
        AcpiEnterSleepStatePrep(5);
        interrupts_disable();
        AcpiEnterSleepState(5);
    }
    else if (strcmp(Info->HardwareId.String, "PNP0C0E") == 0 && value == 0x80) {
        kprintf("ACPI: Sleep button pressed, entering S3!\n");
        sleep(1000);
        AcpiEnterSleepStatePrep(3);
        interrupts_disable();
        AcpiEnterSleepState(3);
    }
}

static UINT32 acpi_button(void *Context) {
    kprintf("ACPI: Button fixed triggered\n");
    ACPI_STATUS status = AcpiReset();
    kprintf("ACPI: Reset status: %d", status);
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
    kprintf("ACPI: LATE\n");
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
