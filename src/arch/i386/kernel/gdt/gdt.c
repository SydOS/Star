#include <main.h>
#include <kprint.h>
#include <arch/i386/kernel/gdt.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load(uint32_t ptr);

// Create a GDT.
gdt_entry_t gdt[GDT_ENTRIES];
gdt_ptr_t gdtPtr;

// Sets up a GDT descriptor.
static void gdt_set_descriptor(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	// Set up the descriptor base address.
   	gdt[num].baseLow = (base & 0xFFFF);
   	gdt[num].baseMiddle = (base >> 16) & 0xFF;
   	gdt[num].baseHigh = (base >> 24) & 0xFF;

	// Set up the descriptor limits.
   	gdt[num].limitLow = (limit & 0xFFFF);
  	gdt[num].granularity = (limit >> 16) & 0x0F;

	// Set up granularity and access flags.
  	gdt[num].granularity |= gran & 0xF0;
   	gdt[num].access = access;
}

// Loads the GDT.
void gdt_load() {
	// Load the GDT into the processor.
   	_gdt_load((uint32_t)&gdtPtr);
	kprintf("GDT: Loaded at 0x%X.\n", &gdtPtr);
}

// Initializes the GDT.
void gdt_init() {
	// Set up the GDT pointer and limit.
	gdtPtr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
	gdtPtr.base = (uint32_t)&gdt;

	gdt_set_descriptor(0, 0, 0, 0, 0);                // Null segment.
	gdt_set_descriptor(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment.
	gdt_set_descriptor(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment.
	gdt_set_descriptor(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment.
	gdt_set_descriptor(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment.

	// Load the GDT.
   	gdt_load();
	kprintf("GDT: Initialized!\n");
}
