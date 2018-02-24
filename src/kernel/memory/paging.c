#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>

// http://www.rohitab.com/discuss/topic/31139-tutorial-paging-memory-mapping-with-a-recursive-page-directory/

static page_t *kernPageTable;

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

void paging_init() {
	// Create table for kernel.
	page_t kernelPageDirAddr = paging_create_page_directory(PAGING_PAGE_READWRITE);
	page_t *kernelPageDirPtr = (page_t*)kernelPageDirAddr;

	for (page_t i = 0; i < PAGE_DIRECTORY_SIZE; i++)
		kernelPageDirPtr[i] = 0;

	// Map 0x0 to end of page stack.
	for (page_t i = 0; i <= memInfo.pageStackEnd; i += PAGE_SIZE_4K)
		paging_map_virtual_to_phys(kernelPageDirPtr, i, i);

	// Map ourself (page directory).
	paging_map_virtual_to_phys(kernelPageDirPtr, kernelPageDirAddr, kernelPageDirAddr);

	// Enable paging.
	asm volatile("mov %%eax, %%cr3": :"a"(kernelPageDirAddr));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");

	//kprintf("Attempting to write to (probably) non-existing page...\n");
	//page_t *test = (page_t*)pmm_pop_page();
	//*test = 0xFF;

	kprintf("Paging initialized!\n");
}
