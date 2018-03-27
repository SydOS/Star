#include <main.h>
#include <tools.h>
#include <io.h>
#include <string.h>
#include <kprint.h>
#include <kernel/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/nmi.h>
#include <kernel/pit.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/kheap.h>
#include <kernel/tasking.h>
#include <kernel/interrupts/smp.h>
#include <kernel/cpuid.h>
#include <driver/vga.h>
#include <driver/floppy.h>
#include <driver/serial.h>
#include <driver/pci.h>
#include <driver/speaker.h>
#include <driver/ps2/ps2.h>
#include <driver/rtc.h>

#include <acpi.h>

// Displays a kernel panic message and halts the system.
void panic(const char *format, ...) {
	// Disable interrupts.
	interrupts_disable();

    // Get args.
    va_list args;
	va_start(args, format);

	// Show panic.
	kprintf_nlock("\nPANIC:\n");
	kprintf_va(format, args);
	kprintf("\n\nHalted.");

	// Halt other processors.
	//lapic_send_nmi_all();

	// Halt forever.
	asm volatile ("hlt");
	while (true);
}

/**
 * The main function for the kernel, called from boot.asm
 */
void kernel_main() {
	vga_disable_cursor();
	
	// Initialize serial for logging.
	serial_init();

	vga_initialize();
	vga_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
	vga_writes("   _____           _  ____   _____ \n");
	vga_writes("  / ____|         | |/ __ \\ / ____|\n");
	vga_writes(" | (___  _   _  __| | |  | | (___  \n");
	vga_writes("  \\___ \\| | | |/ _` | |  | |\\___ \\ \n");
	vga_writes("  ____) | |_| | (_| | |__| |____) |\n");
	vga_writes(" |_____/ \\__, |\\__,_|\\____/|_____/ \n");
	vga_writes("          __/ |                    \n");
	vga_writes("         |___/                     \n");
	vga_writes("Copyright (c) Sydney Erickson 2017 - 2018\n");

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	gdt_init();

	// -------------------------------------------------------------------------
	// MEMORY RELATED STUFF
	vga_setcolor(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
	
	// Initialize memory.
	pmm_init();
    paging_init();
	kheap_init();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	// Initialize IDT, ACPI, and interrupts.
	idt_init();
	acpi_init();
	interrupts_init();
	kprintf("Enabling NMI...\n");
	NMI_enable();
    
    // Enable interrupts.
	interrupts_enable();
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    kprintf("INTERRUPTS ARE ENABLED\n");
    vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);


	kprintf("Setting up PIT...\n");
    pit_init();



	kprintf("Initializing PS/2...\n");
	ps2_init();

	// Initialize SMP.
	smp_init();

	// Start up tasking and create kernel task.
	kprintf("Starting tasking...\n");
	tasking_init();

	// We should never get here.
	panic("Tasking failed to start!\n");
}

void hmmm_thread() {
	while (1) { 
		kprintf("hmm(): %u seconds\n", pit_ticks() / 1000);
		sleep(2000);
	 }
}

void *test2(ACPI_HANDLE ObjHandle,
UINT32 Level,
void *Context) {
	ACPI_STATUS Status;
ACPI_DEVICE_INFO *Info;
ACPI_BUFFER Path;
char Buffer[256];
Path.Length = sizeof (Buffer);
Path.Pointer = Buffer;

	Status = AcpiGetName(ObjHandle, ACPI_SINGLE_NAME, &Path);
	Status = AcpiGetObjectInfo(ObjHandle, &Info);

	if (strcmp(Buffer, "_PIC") == 0) {
		kprintf("Found _PIC\n");

		ACPI_OBJECT_LIST list = {};
		ACPI_OBJECT obj = {};
		obj.Integer.Value = 1;
		obj.Type = ACPI_TYPE_INTEGER;
		list.Count = 1;
		list.Pointer = &obj;

		Status = AcpiGetName(ObjHandle, ACPI_FULL_PATHNAME, &Path);
		kprintf("Status: 0x%X\n", Status);
		Status = AcpiEvaluateObject(ObjHandle, Buffer, &list, NULL);
		kprintf("Status: 0x%X\n", Status);
	}
	return NULL;
}

void *test(ACPI_HANDLE ObjHandle,
UINT32 Level,
void *Context) {
	ACPI_STATUS Status;
ACPI_DEVICE_INFO *Info;
ACPI_BUFFER Path;
char Buffer[256];
Path.Length = sizeof (Buffer);
Path.Pointer = Buffer;

	Status = AcpiGetName(ObjHandle, ACPI_FULL_PATHNAME, &Path);
	Status = AcpiGetObjectInfo(ObjHandle, &Info);

	kprintf ("%s\n", Path.Pointer);
	kprintf (" HID: %s, ADR: %.8llX\n", Info->HardwareId.String, Info->Address);

	//AcpiWalkNamespace(ACPI_TYPE_INTEGER, ObjHandle, 300, test2, NULL, NULL, NULL);

	ACPI_BUFFER bufer = {};
	bufer.Length = ACPI_ALLOCATE_BUFFER;
	//bufer.Pointer = kheap_alloc(ACPI_ALLOCATE_BUFFER);
	Status = AcpiGetIrqRoutingTable(ObjHandle, &bufer);

	for (ACPI_PCI_ROUTING_TABLE *table = bufer.Pointer; ((uintptr_t)table < (uintptr_t)bufer.Pointer + bufer.Length) && table->Length; table = (uintptr_t)table + table->Length) {
		kprintf("IRQ: Pin 0x%X, Address 0x%llX, Source index: %d, Source: 0x%X\n", table->Pin, table->Address, table->SourceIndex, table->Source[3]);
	}

	ACPI_PCI_ROUTING_TABLE *first = bufer.Pointer;
	ACPI_PCI_ROUTING_TABLE *second = bufer.Pointer + first->Length;

	return NULL;
}

void kernel_late() {
	kprintf("Adding second task...\n");
	tasking_add_process(tasking_create_process("hmmm", (uintptr_t)hmmm_thread));
	kprintf("Starting tasking...\n");

		acpi_late_init();

	ACPI_HANDLE sysHandle;
	//AcpiNameToHandle(0, NS_SYSTEM_BUS, &sysHandle);
	//AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 500000, test, NULL, NULL, NULL);
	// Enable APIC modes.
	//AcpiWalkNamespace(ACPI_TYPE_METHOD, ACPI_ROOT_OBJECT, -1, test2, NULL, NULL, NULL);

	ACPI_OBJECT_LIST list = {};
		ACPI_OBJECT obj = {};
		obj.Integer.Value = 1;
		obj.Type = ACPI_TYPE_INTEGER;
		list.Count = 1;
		list.Pointer = &obj;

	kprintf("PIC\n");
	//ACPI_STATUS Status = AcpiEvaluateObject(NULL, "\\_PIC", &list, NULL);
	//kprintf("Status: 0x%X\n", Status);

	AcpiGetDevices("PNP0A03", test, NULL, NULL);

	// Initialize PS/2.
	vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);	

	
	// Initialize floppy.
	vga_setcolor(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
	//floppy_init();


	//ata_init();

    vga_enable_cursor();

	vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);

	// Print CPUID info.
	cpuid_print_capabilities();

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);


	kprintf("Current uptime: %i milliseconds.\n", pit_ticks());
	
	vga_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
	kprintf("Kernel is located at 0x%p!\n", memInfo.kernelStart);
	kprintf("Detected usable RAM: %uMB\n", memInfo.memoryKb / 1024);
	if (memInfo.paeEnabled)
		kprintf("PAE enabled!\n");
	if (memInfo.nxEnabled)
		kprintf("NX enabled!\n");

	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	rtc_init();
	kprintf("24 hour time: %d, binary input: %d\n", rtc_settings->twentyfour_hour_time, rtc_settings->binary_input);
	kprintf("%d:%d:%d %d/%d/%d\n", rtc_time->hours, rtc_time->minutes, rtc_time->seconds, rtc_time->month, rtc_time->day, rtc_time->year);

	pci_init();
	pci_check_busses(0);

    vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
	kprintf("root@sydos ~: ");
	serial_writes("root@sydos ~: ");
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    // Ring serial and VGA terminals.
	serial_write('\a');
	vga_putchar('\a');
  
	// If serial isn't present, just loop.
	if (!serial_present()) {
		kprintf("No serial port present for logging, waiting here.");
		while (true);
	}

	for(;;) {
		char c = serial_read();

		if (c == '\r' || c == '\n') {
			vga_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
			kprintf("\nroot@sydos ~: ");
			serial_writes("\nroot@sydos ~: ");
			vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		} else {
			vga_putchar(c);
			serial_write(c);
			vga_trigger_cursor_update();
		}
	}
}
