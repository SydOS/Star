#include <main.h>
#include <multiboot.h>
#include <kernel/pmm.h>

page_t bootPageDirectory[1024]   __attribute__((aligned(PAGE_SIZE_4K))) __attribute__((section(".inittables")));
page_t bootPageLowTable[1024]    __attribute__((aligned(PAGE_SIZE_4K))) __attribute__((section(".inittables")));
page_t bootPageKernelTableA[1024] __attribute__((aligned(PAGE_SIZE_4K))) __attribute__((section(".inittables")));
page_t bootPageKernelTableB[1024] __attribute__((aligned(PAGE_SIZE_4K))) __attribute__((section(".inittables")));

extern uint32_t KERNEL_VIRTUAL_OFFSET;
extern uint32_t KERNEL_PHYSICAL_START;
extern uint32_t KERNEL_PHYSICAL_END;
extern uint32_t KERNEL_VIRTUAL_START;
extern uint32_t KERNEL_VIRTUAL_END;
extern uint32_t KERNEL_INIT_END;

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

    // Determine stack offset and size.
    PAGE_STACK_START = ALIGN_4K(&KERNEL_VIRTUAL_END);
    PAGE_STACK_END = PAGE_STACK_START + memory / 1024;

    // Zero out page directory.
    for (uint32_t i = 0; i < 1024; i++)
        bootPageDirectory[i] = 0;

    // Identity map low memory and init section of kernel.
    page_t currentPage = 0;
    for (uint32_t i = 0; i <= (((uint32_t)&KERNEL_INIT_END) / PAGE_SIZE_4K); i++, currentPage += PAGE_SIZE_4K)
        bootPageLowTable[i] = (currentPage) | 0x03;

    // Map rest of kernel to virtual space.
    uint32_t startIndex = (((uint32_t)&KERNEL_VIRTUAL_START) % 0x400000 / PAGE_SIZE_4K);
    currentPage = (uint32_t)&KERNEL_PHYSICAL_START;
    for (uint32_t i = startIndex; i <= (((uint32_t)&KERNEL_VIRTUAL_END) % 0x400000 / PAGE_SIZE_4K); i++, currentPage += PAGE_SIZE_4K)
        bootPageKernelTableA[i] = currentPage | 0x03;
    
    // Set locations of tables in directory.
    bootPageDirectory[0] = ((uint32_t)bootPageLowTable) | 0x03;
    bootPageDirectory[((uint32_t)&KERNEL_VIRTUAL_START) / 0x400000] = ((uint32_t)bootPageKernelTableA) | 0x03;

    // Map out page stack in virtual space.
    startIndex = (PAGE_STACK_START % 0x400000 / PAGE_SIZE_4K);
    uint32_t startIndexTable = PAGE_STACK_START / 0x400000;
    uint32_t endIndexTable = PAGE_STACK_END / 0x400000;
    uint32_t endIndex = startIndexTable == endIndexTable ? (PAGE_STACK_END % 0x400000 / PAGE_SIZE_4K) : 1023;
    currentPage = PAGE_STACK_START;

    for (uint32_t i = startIndex; i <= endIndex; i++, currentPage += PAGE_SIZE_4K)
        bootPageKernelTableA[i] = (currentPage - ((uint32_t)&KERNEL_VIRTUAL_OFFSET)) | 0x03;

    // If stack crosses 4KB boundary, map second portion.
    if (startIndexTable != endIndexTable) {
        endIndex = (PAGE_STACK_END % 0x400000 / PAGE_SIZE_4K);
        for (uint32_t i = 0; i <= endIndex; i++, currentPage += PAGE_SIZE_4K)
            bootPageKernelTableB[i] = (currentPage - ((uint32_t)&KERNEL_VIRTUAL_OFFSET)) | 0x03;
        bootPageDirectory[endIndexTable] = ((uint32_t)bootPageKernelTableB) | 0x03;
    }
    
    // Enable paging.
	asm volatile("mov %%eax, %%cr3": :"a"((uint32_t)bootPageDirectory));	
	asm volatile("mov %cr0, %eax");
	asm volatile("orl $0x80000000, %eax");
	asm volatile("mov %eax, %cr0");
}
