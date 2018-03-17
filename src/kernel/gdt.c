#include <main.h>
#include <kprint.h>
#include <kernel/gdt.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load();

// Create a GDT.
gdt_entry_t gdt[GDT_ENTRIES];
gdt_ptr_t gdtPtr;

// Sets up a GDT descriptor.
static void gdt_set_descriptor(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
	// Set up the descriptor base address.
   	gdt[num].baseLow = (base & 0xFFFF);
   	gdt[num].baseMiddle = (base >> 16) & 0xFF;
   	gdt[num].baseHigh = (base >> 24) & 0xFF;

	// Set up the descriptor limits.
   	gdt[num].limitLow = (limit & 0xFFFF);
  	gdt[num].granularity = (limit >> 16) & 0x0F;

	// Set up granularity and access flags.
  	gdt[num].granularity |= flags & 0xF0;
   	gdt[num].access = access;
}

// Loads the GDT.
void gdt_load() {
	// Load the GDT into the processor.
   	_gdt_load();
	kprintf("GDT: Loaded at 0x%X.\n", &gdtPtr);
}

// Initializes the GDT.
void gdt_init() {
	kprintf("GDT: Initializing...\n");

	// Set up the GDT pointer and limit.
	gdtPtr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
	gdtPtr.base = (uintptr_t)&gdt;
	gdtPtr.end = 0x80;

	gdt_set_descriptor(0, 0, 0, 0, 0);                			// Null segment.
	gdt_set_descriptor(1, 0, 0xFFFFFFFF, 0x9A, GDT_CODE_FLAGS); // Code segment.
	gdt_set_descriptor(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 			// Data segment.
	gdt_set_descriptor(3, 0, 0xFFFFFFFF, 0xFA, GDT_CODE_FLAGS); // User mode code segment.
	gdt_set_descriptor(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 			// User mode data segment.

	//gdt_set_descriptor(5, 0x80000, 0x67, 0xE9, 0x00); // User mode data segment.
	//gdt_set_descriptor(6, 0, 0x80, 0x00, 0x00); // User mode data segment.);

	// Load the GDT.
   	gdt_load();
	kprintf("GDT: Initialized!\n");
}
