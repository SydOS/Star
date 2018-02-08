#include <main.h>
#include <logging.h>
#include <driver/vga.h>

/**
 * Set GDT Register (GDTR)
 */
extern void _set_gdtr();
/**
 * Reload GDT segments
 */
extern void _reload_segments();

static uint32_t gdt_pointer = 0x806;
static uint32_t gdtr_loc = 0x800;
static uint32_t gdt_size = 0;

/**
 * Add a descriptor to the GDT table
 * @param id   ID of GDT entry to add
 * @param desc Description
 */
void gdt_add_descriptor(uint8_t id, uint64_t desc) {
	uint32_t loc = gdt_pointer + sizeof(uint64_t)*id;
	*(uint64_t*)loc = desc;
	gdt_size += sizeof(desc);
}

/**
 * Set a GDT entry
 */
void gdt_set_descriptor() {
	*(uint16_t*)gdtr_loc = (gdt_size - 1) & 0x0000FFFF;
	gdtr_loc += 2;
	*(uint32_t*)gdtr_loc = gdt_pointer;
	_set_gdtr();
	_reload_segments();
}

/**
 * Initialize the GDT table
 */
void gdt_init() {
	gdt_add_descriptor(0, 0);
	gdt_add_descriptor(1, 0x00CF9A000000FFFF);
	gdt_add_descriptor(2, 0x00CF92000000FFFF);
	gdt_add_descriptor(3, 0x008FFA000000FFFF);
	gdt_set_descriptor(4, 0x008FF2000000FFFF);
	log("GDT initialized!\n");
}