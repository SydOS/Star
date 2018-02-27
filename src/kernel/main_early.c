#include <main.h>
#include <multiboot.h>
#include <kernel/pmm.h>

extern uint32_t KERNEL_VIRTUAL_END;
extern uint32_t KERNEL_PHYSICAL_END;
extern uint32_t KERNEL_INIT_END;

uint32_t KERNEL_PAGE_DIRECTORY __attribute__((section(".inittables")));
uint32_t PAGE_STACK_START __attribute__((section(".inittables")));
uint32_t PAGE_STACK_END __attribute__((section(".inittables")));

// Performs pre-boot tasks.
__attribute__((section(".init"))) void kernel_main_early(uint32_t mbootMagic, multiboot_info_t* mbootInfo) {
	// Ensure multiboot magic value is good and a memory map is present.
	if ((mbootMagic != MULTIBOOT_BOOTLOADER_MAGIC) || ((mbootInfo->flags & MULTIBOOT_INFO_MEM_MAP) == 0))
        return;

    // Count up memory.
    uint64_t memory = 0;
    uint32_t base = mbootInfo->mmap_addr;
	uint32_t end = base + mbootInfo->mmap_length;
	for(multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base; base < end; base += entry->size + sizeof(uint32_t)) {
		entry = (multiboot_memory_map_t*)base;
		if(entry->type == 1 && ((uint32_t)entry->addr) > 0)
			memory += entry->len;
	}

    // Determine location of kernel page directory.
    KERNEL_PAGE_DIRECTORY = ALIGN_4K((uint32_t)&KERNEL_VIRTUAL_END);
    page_t *kernelPageDirectory = (page_t*)(KERNEL_PAGE_DIRECTORY - ((uint32_t)&KERNEL_VIRTUAL_OFFSET));
    for (uint32_t i = 0; i < 1024; i++)
        kernelPageDirectory[i] = 0;

    // Determine stack offset and size.
    PAGE_STACK_START = ALIGN_4K(KERNEL_PAGE_DIRECTORY);
    PAGE_STACK_END = PAGE_STACK_START + memory / 1024;

    // Identity map low memory and init section of kernel.
    page_t currentPage = 0;
    page_t *bootPageLowTable = (page_t*)ALIGN_4K(PAGE_STACK_END - (uint32_t)&KERNEL_VIRTUAL_OFFSET);
    for (uint32_t i = 0; i <= (((uint32_t)&KERNEL_INIT_END) / PAGE_SIZE_4K); i++, currentPage += PAGE_SIZE_4K)
        bootPageLowTable[i] = (currentPage) | 0x03;
    kernelPageDirectory[0] = ((uint32_t)bootPageLowTable) | 0x03;

    // Create first table and add it to directory.
    page_t *bootKernelTable = (page_t*)ALIGN_4K((uint32_t)bootPageLowTable);
    kernelPageDirectory[(((uint32_t)&KERNEL_VIRTUAL_OFFSET) / 0x400000)] = ((uint32_t)bootKernelTable) | 0x03;
    
    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    currentPage = 0;
    for (uint32_t i = 0; i <= (((PAGE_STACK_END - ((uint32_t)&KERNEL_VIRTUAL_OFFSET))) / PAGE_SIZE_4K); i++, currentPage += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (i > 0 && i % 1024 == 0) {
            // Create a new table after the previous one.        
            uint32_t prevAddr = (uint32_t)bootKernelTable;
            bootKernelTable = (page_t*)ALIGN_4K(prevAddr);

            // Increase offset and add new table to directory.
            offset++;
            kernelPageDirectory[(((uint32_t)&KERNEL_VIRTUAL_OFFSET) / 0x400000)+offset] = ((uint32_t)bootKernelTable) | 0x03;
        }
        bootKernelTable[i-(offset*1024)] = currentPage | 0x03;
    }

    // Enable paging.
	asm volatile("mov %%eax, %%cr3": :"a"((uint32_t)kernelPageDirectory));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");
}
