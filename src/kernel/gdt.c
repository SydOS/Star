#include <main.h>
#include <kprint.h>
#include <kernel/gdt.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load();

// Represents the 32-bit GDT and pointer to it. This is used as
// the system GDT on 32-bit systems and when starting up other processors,
gdt_entry_t gdt32[GDT32_ENTRIES];
gdt_ptr_t gdt32Ptr;

#ifdef X86_64
// Represents the 64-bit GDT and pointer to it. Only used in 64-bit mode.
gdt_entry_t gdt64[GDT64_ENTRIES];
gdt_ptr_t gdt64Ptr;
#endif

// Sets up a GDT descriptor.
static void gdt_set_descriptor(gdt_entry_t *gdt, int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
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
	kprintf("32-bit GDT: Loaded at 0x%X.\n", &gdt32Ptr);

#ifdef X86_64
	kprintf("64-bit GDT: Loaded at 0x%X.\n", &gdt64Ptr);
#endif
}

// Initializes the GDT.
void gdt_init() {
	kprintf("GDT: Initializing...\n");

	// Set up the 32-bit GDT pointer and limit.
	gdt32Ptr.limit = (sizeof(gdt_entry_t) * GDT32_ENTRIES) - 1;
	gdt32Ptr.base = (uintptr_t)&gdt32;

	// Add descriptors to 32-bit GDT.
	gdt_set_descriptor(gdt32, 0, 0, 0, 0, 0);                			// Null segment.
	gdt_set_descriptor(gdt32, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 			// Code segment.
	gdt_set_descriptor(gdt32, 2, 0, 0xFFFFFFFF, 0x92, 0xCF); 			// Data segment.
	gdt_set_descriptor(gdt32, 3, 0, 0xFFFFFFFF, 0xFA, 0xCF); 			// User mode code segment.
	gdt_set_descriptor(gdt32, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 			// User mode data segment.

#ifdef X86_64
	// Set up the 64-bit GDT pointer and limit.
	gdt64Ptr.limit = (sizeof(gdt_entry_t) * GDT64_ENTRIES) - 1;
	gdt64Ptr.base = (uintptr_t)&gdt64;

	// Add descriptors to 64-bit GDT.
	gdt_set_descriptor(gdt64, 0, 0, 0, 0, 0);                			// Null segment.
	gdt_set_descriptor(gdt64, 1, 0, 0xFFFFFFFF, 0x9A, 0xAF); 			// Code segment.
	gdt_set_descriptor(gdt64, 2, 0, 0xFFFFFFFF, 0x92, 0xCF); 			// Data segment.
	gdt_set_descriptor(gdt64, 3, 0, 0xFFFFFFFF, 0xFA, 0xAF); 			// User mode code segment.
	gdt_set_descriptor(gdt64, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 			// User mode data segment.

	//gdt_set_descriptor(5, 0x80000, 0x67, 0xE9, 0x00); // User mode data segment.
	//gdt_set_descriptor(6, 0, 0x80, 0x00, 0x00); // User mode data segment.);
#endif

	// Load the GDT.
   	gdt_load();
	kprintf("GDT: Initialized!\n");
}
