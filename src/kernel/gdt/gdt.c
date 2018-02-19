#include <main.h>
#include <kprint.h>
#include <kernel/gdt.h>

extern void _gdt_load(uint32_t ptr);


gdt_entry_t gdt[5];
gdt_ptr_t   gdt_ptr;

// Sets up a GDT descriptor.
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	// Set up the descriptor base address.
   	gdt[num].base_low    = (base & 0xFFFF);
   	gdt[num].base_middle = (base >> 16) & 0xFF;
   	gdt[num].base_high   = (base >> 24) & 0xFF;

	// Set up the descriptor limits.
   	gdt[num].limit_low   = (limit & 0xFFFF);
  	gdt[num].granularity = (limit >> 16) & 0x0F;

	// Set up granularity and access flags.
  	gdt[num].granularity |= gran & 0xF0;
   	gdt[num].access      = access;
} 

// Initializes the GDT.
void gdt_init()
{
	// Set up the GDT pointer and limit.
	gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
	gdt_ptr.base  = (uint32_t)&gdt;

	gdt_set_gate(0, 0, 0, 0, 0);                // Null segment.
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment.
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment.
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment.
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment.

	// Load the GDT into the processor.
   	_gdt_load((uint32_t)&gdt_ptr);
	kprintf("GDT initialized!\n");
}
