#include <main.h>
#include <driver/vga.h>

extern void _set_gdtr();
extern void _reload_segments();

static uint32_t gdt_pointer = 0x806;
static uint32_t gdtr_loc = 0x800;
static uint32_t gdt_size = 0;

int gdt_add_descriptor(uint8_t id, uint64_t desc) {
	uint32_t loc = gdt_pointer + sizeof(uint64_t)*id;
	*(uint64_t*)loc = desc;
	gdt_size += sizeof(desc);
	return 0;
}

int gdt_set_descriptor() {
	*(uint16_t*)gdtr_loc = (gdt_size - 1) & 0x0000FFFF;
	gdtr_loc += 2;
	*(uint32_t*)gdtr_loc = gdt_pointer;
	_set_gdtr();
	_reload_segments();
	return 0;
}

void gdt_init() {
	gdt_add_descriptor(0, 0);
	gdt_add_descriptor(1, 0x00CF9A000000FFFF);
	gdt_add_descriptor(2, 0x00CF92000000FFFF);
	gdt_add_descriptor(3, 0x008FFA000000FFFF);
	gdt_set_descriptor(4, 0x008FF2000000FFFF);
	vga_writes("GDT initialized\n");
}