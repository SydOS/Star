#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <arch/i386/kernel/interrupts.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>

// http://www.rohitab.com/discuss/topic/31139-tutorial-paging-memory-mapping-with-a-recursive-page-directory/

page_t kernelPageDir[1024] __attribute__((aligned(PAGE_SIZE_4K)));
page_t lowPageTable[1024] __attribute__((aligned(PAGE_SIZE_4K)));
page_t kernelPageTable[1024] __attribute__((aligned(PAGE_SIZE_4K)));

static uint32_t* page_directory = 0;
static uint32_t page_dir_loc = 0;
static uint32_t* last_page = 0;

static uint32_t paging_calculate_table(page_t virtAddr) {
	return virtAddr / 0x400000;
}

static uint32_t paging_calculate_entry(page_t virtAddr) {
	return virtAddr % 0x400000 / 0x1000;
}

void paging_map_virtual_to_phys(page_t *directory, page_t virt, page_t phys) {
	//kprintf("Mapping virtual 0x%X to physical 0x%X...\n", virt, phys);

	// Calculate table and entry of virtual address.
	uint32_t tableIndex = paging_calculate_table(virt);
	uint32_t entryIndex = paging_calculate_entry(virt);

	// Get address of table from directory.
	// If there isn't one, create one.
	// Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.	
	if (MASK_PAGE_4K(directory[tableIndex]) == 0)
		directory[tableIndex] = pmm_pop_page() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
	page_t *table = (page_t*)MASK_PAGE_4K(directory[tableIndex]);

	// Add address to table.
	table[entryIndex] = phys | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
}

void paging_enable()
{
	asm volatile("mov %%eax, %%cr3": :"a"(page_dir_loc));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");
}

page_t paging_create_page_directory(uint16_t flags) {
	// Pop page for page directory.
	page_t pageDir = pmm_pop_page();
	kprintf("Popped page 0x%X for page directory.\n", pageDir);

	// Create pointer and zero out page.
	page_t *pageDirPtr = (page_t*)pageDir;
	for (page_t i = 0; i < PAGE_DIRECTORY_SIZE; i++)
		pageDirPtr[i] = 0 | flags;
		
	return pageDir;
}

static void paging_pagefault_handler(registers_t *regs) {
	

	page_t addr;
	asm volatile ("mov %%cr2, %0" : "=r"(addr));

	panic("Page fault at 0x%X!\n", addr);
}

void paging_init(mem_info_t memInfo) {
	// Create table for kernel.
	for (page_t i = 0; i < PAGE_DIRECTORY_SIZE; i++)
		kernelPageDir[i] = 0;

	//kernelPageDirAddr = paging_create_page_directory(PAGING_PAGE_READWRITE);
	//kernelPageDirPtr = (page_t*)kernelPageDirAddr;

//	for (page_t i = 0; i < PAGE_DIRECTORY_SIZE; i++)
		//kernelPageDirPtr[i] = 0;

	// Map 0x0 to end of page stack.
	uint32_t currentPage = 0;
	for (page_t i = 0; i < 256; i++, currentPage += PAGE_SIZE_4K) {
	//	paging_map_virtual_to_phys(kernelPageDir, i, i);
	// Calculate table and entry of virtual address.
	//uint32_t entryIndex = paging_calculate_entry(i);

	// Get address of table from directory.
	// If there isn't one, create one.
	// Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.	
	//if (MASK_PAGE_4K(directory[tableIndex]) == 0)
		//directory[tableIndex] = pmm_pop_page() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

	// Add address to table.
	lowPageTable[i] = currentPage | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
	}

	currentPage = 0x00000000;
	for (page_t i = 0; i < memInfo.pageStackEnd / PAGE_SIZE_4K; i++, currentPage += PAGE_SIZE_4K) {
	//	paging_map_virtual_to_phys(kernelPageDir, i, i);
	// Calculate table and entry of virtual address.
	//uint32_t entryIndex = paging_calculate_entry(i);

	// Get address of table from directory.
	// If there isn't one, create one.
	// Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.	
	//if (MASK_PAGE_4K(directory[tableIndex]) == 0)
		//directory[tableIndex] = pmm_pop_page() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

	// Add address to table.
	kernelPageTable[i] = currentPage | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
	}

	kernelPageDir[0] = (((uint32_t)lowPageTable) - 0xC0000000) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
	kernelPageDir[768] = (((uint32_t)kernelPageTable) - 0xC0000000) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

	// Map ourself (page directory).
	//paging_map_virtual_to_phys(kernelPageDir, (uint32_t)kernelPageDir, (uint32_t)kernelPageDir);

	// Wire up handler for page faults.
	interrupts_isr_install_handler(ISR_EXCEPTION_PAGE_FAULT, paging_pagefault_handler);

	// Enable paging.
	asm volatile("mov %%eax, %%cr3": :"a"(((uint32_t)kernelPageDir) - 0xC0000000));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");

	//kprintf("Attempting to write to (probably) non-existing page...\n");
	//page_t *test = (page_t*)0xFFFFFF;
	//*test = 0xFF;

	kprintf("Paging initialized!\n");
}
