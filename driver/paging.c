#include <main.h>
#include <logging.h>

static uint32_t* page_directory = 0;
static uint32_t page_dir_loc = 0;
static uint32_t* last_page = 0;

void paging_map_virtual_to_phys(uint32_t virt, uint32_t phys)
{
	uint16_t id = virt >> 22;
	for(int i = 0; i < 1024; i++)
	{
		last_page[i] = phys | 3;
		phys += 4096;
	}
	page_directory[id] = ((uint32_t)last_page) | 3;
	last_page = (uint32_t *)(((uint32_t)last_page) + 4096);

	char virt_s[32], id_s[32], phys_s[32];
	itoa((uint32_t)&virt, virt_s, 16);
	itoa((uint32_t)&id, id_s, 16);
	itoa((uint32_t)&phys, phys_s, 16);

	log("Mapping 0x");
	log(virt_s);
	log(" (");
	log(id_s);
	log(") to 0x");
	log(phys_s);
	log("\n");
}

void paging_enable()
{
	asm volatile("mov %%eax, %%cr3": :"a"(page_dir_loc));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");
}

void paging_initialize() {
	page_directory = (uint32_t*)0x400000;
	page_dir_loc = (uint32_t)page_directory;
	last_page = (uint32_t *)0x404000;
	for (int i = 0; i < 1024; i++) {
		page_directory[i] = 0 | 2;
	}

	paging_map_virtual_to_phys(0, 0);
	paging_map_virtual_to_phys(0x400000, 0x400000);
	paging_enable();
	log("Paging initialized!\n");
}