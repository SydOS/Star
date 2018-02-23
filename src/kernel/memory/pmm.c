#include <main.h>
#include <kprint.h>
#include <multiboot.h>
#include <string.h>
#include <math.h>
#include <kernel/pmm.h>

// Kernel's starting and ending addresses in RAM.
extern uint32_t kernel_end;
extern uint32_t kernel_base;

// Used to store info about memory in the system.
mem_info_t memInfo;

// Page stack, stores addresses to 4K pages in physical memory.
static page_t *pageStack;
static uint32_t pagesAvailable = 0;

// Pushes a page to the stack.
void pmm_push_page(page_t page) {
    // Increment stack pointer and check its within bounds.
    pageStack++;
    if (((uint32_t)(pageStack)) < memInfo.pageStackStart || ((uint32_t)(pageStack)) >= memInfo.pageStackEnd)
    {
        kprintf("Error!\n");
        while (true);
    }

    // Push page to stack.
    *pageStack = page;
    pagesAvailable++;
}

// Pops a page off the stack.
page_t pmm_pop_page() {
    // Get page off stack.
    page_t page = *pageStack;

    // Verify there are pages.
    if (pagesAvailable == 0)
        panic("No more pages!\n");

    // Decrement stack and return page.
    pageStack--;
    pagesAvailable--;
    return page;
}

// Builds the stack.
static void pmm_build_stack() {
    // Initialize stack.
    kprintf("Initializing page stack at 0x%X...\n", memInfo.pageStackStart);
    pageStack = (page_t*)(memInfo.pageStackStart);
    memset(pageStack, 0, memInfo.pageStackEnd - memInfo.pageStackStart);

    // Perform memory test on stack areas.
	kprintf("Testing 4M of memory at 0x%X...\n", memInfo.pageStackStart);
	for (page_t i = 0; i < memInfo.pageStackEnd - memInfo.pageStackStart; i++)
		pageStack[i] = i;

    bool pass = true;
	for (page_t i = 0; i < memInfo.pageStackEnd - memInfo.pageStackStart; i++)
		if (pageStack[i] != i) {
			pass = false;
			break;
		}
	kprintf("Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("Memory test of page stack area failed.\n");

    // Build stack of free pages.
	for (multiboot_memory_map_t *entry = memInfo.mmap; (uint32_t)entry < (uint32_t)memInfo.mmap + memInfo.mmapLength;
		entry = (multiboot_memory_map_t*)((uint32_t)entry + entry->size + sizeof(entry->size))) {
		
		// If not available memory or 64-bit address (greater than 4GB), skip over.
		if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr & 0xFFFFFFFF00000000)
			continue;

        // Add pages to stack.
        uint32_t pageBase = ALIGN_4K(entry->addr);	
        kprintf("Adding pages in 0x%X!\n", pageBase);			
		for (uint32_t i = 0; i < entry->len / PAGE_SIZE_4K; i++) {
			uint32_t addr = pageBase + (i * PAGE_SIZE_4K);

			// If the address is in conventional memory (low memory), or is reserved by
			// the kernel, Multiboot header, or the page stacks, don't mark it free.
			if (addr <= 0x100000 || (addr >= memInfo.kernelStart && addr <= memInfo.kernelEnd) || 
                (addr >= memInfo.pageDirStart && addr <= memInfo.pageDirEnd) || (addr >= memInfo.pageTableStart && addr <= memInfo.pageTableEnd) ||
                (addr >= memInfo.pageStackStart && addr <= memInfo.pageStackEnd))
				continue;

            // Add page to stack.
            pmm_push_page(addr);
		}       
	}

    kprintf("Added %u pages!\n", pagesAvailable);
}

// Initializes the physical memory manager.
void pmm_init(multiboot_info_t* mbootInfo) {
	// Ensure a memory map is present.
	if ((mbootInfo->flags & MULTIBOOT_INFO_MEM_MAP) == 0)
		panic("NO MULTIBOOT MEMORY MAP FOUND!\n");

	// Store away Multiboot info.
	memInfo.mbootInfo = mbootInfo;
	memInfo.mmap = (multiboot_memory_map_t*)mbootInfo->mmap_addr;
	memInfo.mmapLength = mbootInfo->mmap_length;

	// Store where the Multiboot info structure actually resides in memory.
	memInfo.mbootStart = (uint32_t)mbootInfo;
	memInfo.mbootEnd = (uint32_t)(mbootInfo + sizeof(multiboot_info_t));

	// Store where the kernel is. These come from the linker.
	memInfo.kernelStart = (uint32_t)&kernel_base;
	memInfo.kernelEnd = (uint32_t)&kernel_end;

    // Calculate page directory and page table locations on the first
    // 4KB aligned addresses after the kernel.
    memInfo.pageDirStart = ALIGN_4K(memInfo.kernelEnd);
    memInfo.pageDirEnd = memInfo.pageDirStart + PAGE_DIRECTORY_SIZE;
    memInfo.pageTableStart = memInfo.pageDirEnd;
    memInfo.pageTableEnd = memInfo.pageTableStart + PAGE_TABLE_SIZE;

    // Calculate page stack location, located after the page directory and table.
    memInfo.pageStackStart = memInfo.pageTableEnd;
    memInfo.pageStackEnd = memInfo.pageStackStart + PAGE_TABLE_SIZE * 1024;

	// Print summary.
	kprintf("Kernel start: 0x%X | Kernel end: 0x%X\n", memInfo.kernelStart, memInfo.kernelEnd);
	kprintf("Multiboot info start: 0x%X | Multiboot info end: 0x%X\n", memInfo.mbootStart, memInfo.mbootEnd);
	kprintf("Page directory start: 0x%X | Page directory end: 0x%X\n", memInfo.pageDirStart, memInfo.pageDirEnd);
    kprintf("Page table start: 0x%X | Page table end: 0x%X\n", memInfo.pageTableStart, memInfo.pageTableEnd);
    kprintf("Page stack start: 0x%X | Page stack end: 0x%X\n", memInfo.pageStackStart, memInfo.pageStackEnd);

	kprintf("Physical memory map:\n");
	uint64_t memory = 0;

	uint32_t base = mbootInfo->mmap_addr;
	uint32_t end = base + mbootInfo->mmap_length;
	multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base;

	for(; base < end; base += entry->size + sizeof(uint32_t))
	{
		entry = (multiboot_memory_map_t*)base;

		// Print out info.
		kprintf("region start: 0x%llX length: 0x%llX type: 0x%X\n", entry->addr, entry->len, (uint64_t)entry->type);

		if(entry->type == 1 && ((uint32_t)entry->addr) > 0)
			memory += entry->len;
	}

	memory = memory / 1024 / 1024;
	kprintf("Detected RAM: %uMB\n", memory);

    // Build stack.
    pmm_build_stack();

    // Test out stack.
    page_t *page = (page_t*)pmm_pop_page();
    page_t i = 0;
    for (i = 0; i < PAGE_SIZE_4K; i++)
        page[i] = i;

    bool pass = true;
    for (i = 0; i < PAGE_SIZE_4K; i++)
        if (page[i] != i) {
            pass = false;
            break;
        }

    // Push page back to stack.
    pmm_push_page((page_t)&page);

    kprintf("Stack test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("Test of memory page failed.\n");
}
