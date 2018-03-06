#include <main.h>
#include <multiboot.h>
#include <arch/i386/kernel/cpuid.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>

extern uint32_t KERNEL_VIRTUAL_OFFSET;
extern uint32_t KERNEL_VIRTUAL_END;
extern uint32_t KERNEL_PHYSICAL_END;
extern uint32_t KERNEL_INIT_END;

uint32_t KERNEL_PAGE_DIRECTORY __attribute__((section(".inittables")));
uint32_t KERNEL_PAGE_TEMP __attribute__((section(".inittables")));
uint32_t PAGE_FRAME_STACK_START __attribute__((section(".inittables")));
uint32_t PAGE_FRAME_STACK_END __attribute__((section(".inittables")));
bool PAE_ENABLED __attribute__((section(".inittables"))) = false;

extern uint32_t _cpuid_detect_early();

// Performs pre-boot tasks. This function must not call code in other files.
__attribute__((section(".init"))) void kernel_main_early(uint32_t mbootMagic, multiboot_info_t* mbootInfo) {
	// Ensure multiboot magic value is good and a memory map is present.
	if ((mbootMagic != MULTIBOOT_BOOTLOADER_MAGIC) || ((mbootInfo->flags & MULTIBOOT_INFO_MEM_MAP) == 0))
        return;

    // Determine if PAE is supported.
    if (_cpuid_detect_early() != 0) {
        // Ensure the GETFEATURES function is supported.
        uint32_t highestFunction = 0;
        asm volatile ("cpuid" : "=a"(highestFunction) : "a"(CPUID_GETVENDORSTRING), "b"(0), "c"(0), "d"(0));
        if (highestFunction >= CPUID_GETFEATURES) {
            // Check for presence of PAE bit.
            uint32_t edx;
            asm volatile ("cpuid" : "=d"(edx) : "a"(CPUID_GETFEATURES), "b"(0), "c"(0), "d"(0));
            if (edx & CPUID_FEAT_EDX_PAE)
                PAE_ENABLED = true; // PAE is supported.
        }
    }

    // Force PAE off for now.
    PAE_ENABLED = false;

    // Count up memory.
    uint64_t memory = 0;
    uintptr_t base = mbootInfo->mmap_addr;
	uintptr_t end = base + mbootInfo->mmap_length;
	for(multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base; base < end; base += entry->size + sizeof(uintptr_t)) {
		entry = (multiboot_memory_map_t*)base;
		if(entry->type == 1 && ((uint32_t)entry->addr) > 0)
			memory += entry->len;
	}

    // Determine location of kernel page directory for later use.
    // The pre-boot PDPT is also placed here if using PAE.
    KERNEL_PAGE_DIRECTORY = ALIGN_4K((uintptr_t)&KERNEL_VIRTUAL_END);

    // Allocate temp location. This is used for for temp purposes later on.
    // The pre-boot page directory is placed here if using non-PAE mode.
    KERNEL_PAGE_TEMP = ALIGN_4K(KERNEL_PAGE_DIRECTORY);

    // Determine stack offset and size.
    PAGE_FRAME_STACK_START = ALIGN_4K(KERNEL_PAGE_TEMP);
    PAGE_FRAME_STACK_END = PAGE_FRAME_STACK_START + (memory / 1024) * 2; // Space for 64-bit addresses.

    // Can we use PAE?
    if (PAE_ENABLED) {
        // Get pointer to page directory pointer table page and clear it out. The pre-boot PDPT is located after the normal one.
        uint64_t *pageDirectoryPointerTable = ((uint64_t*)(KERNEL_PAGE_DIRECTORY - (uint32_t)&KERNEL_VIRTUAL_OFFSET)) + 4;
        for (uint32_t i = 0; i < PAGE_PAE_PDPT_SIZE; i++)
            pageDirectoryPointerTable[i] = 0;

        // Create our low page directory (0x0 to 0x3FFFFFFF).
        uint64_t *pageLowDirectory = (uint64_t*)ALIGN_4K(PAGE_FRAME_STACK_END - (uint32_t)&KERNEL_VIRTUAL_OFFSET);
        for (uint32_t i = 0; i < PAGE_PAE_DIRECTORY_SIZE; i++)
            pageLowDirectory[i] = 0;
        pageDirectoryPointerTable[0] = (uint32_t)pageLowDirectory | PAGING_PAGE_PRESENT;

        // Create page table for low memory and init section of kernel.
        uint64_t *pageLowTable = (uint64_t*)ALIGN_4K((uint32_t)pageLowDirectory);
        for (uint32_t i = 0; i < PAGE_PAE_TABLE_SIZE; i++)
            pageLowTable[i] = 0;

        // Identity map low memory and init section of kernel.
        for (page_t page = 0; page <= ((uint32_t)&KERNEL_INIT_END); page += PAGE_SIZE_4K)
            pageLowTable[page / PAGE_SIZE_4K] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        pageLowDirectory[0] = (uint32_t)pageLowTable | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

        // Create our kernel page directory (0xC0000000 to 0xFFFFFFFF).
        uint64_t *pageKernelDirectory = (uint64_t*)ALIGN_4K((uint32_t)pageLowTable);
        uint32_t dirIndex = (uint32_t)&KERNEL_VIRTUAL_OFFSET / PAGE_SIZE_1G;
        for (uint32_t i = 0; i < PAGE_PAE_DIRECTORY_SIZE; i++)
            pageKernelDirectory[i] = 0;
        pageDirectoryPointerTable[dirIndex] = (uint32_t)pageKernelDirectory | PAGING_PAGE_PRESENT;

        // Create first table and add it to directory.
        uint64_t *pageKernelTable = (uint64_t*)ALIGN_4K((uint32_t)pageKernelDirectory);
        for (uint32_t i = 0; i < PAGE_PAE_TABLE_SIZE; i++)
            pageKernelTable[i] = 0;
        pageKernelDirectory[((uint32_t)&KERNEL_VIRTUAL_OFFSET - (dirIndex * PAGE_SIZE_1G)) / PAGE_SIZE_2M] =
            (uint32_t)pageKernelTable | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    
        // Map low memory and kernel to higher-half virtual space.
        uint32_t offset = 0;
        for (page_t page = 0; page <= (PAGE_FRAME_STACK_END - (uint32_t)&KERNEL_VIRTUAL_OFFSET); page += PAGE_SIZE_4K) {
            // Have we reached the need to create another table?
            if (page > 0 && page % PAGE_SIZE_2M == 0) {
                // Create a new table after the previous one.        
                uint32_t prevAddr = (uint32_t)pageKernelTable;
                pageKernelTable = (uint64_t*)ALIGN_4K(prevAddr);
                for (uint32_t i = 0; i < PAGE_PAE_TABLE_SIZE; i++)
                    pageKernelTable[i] = 0;

                // Increase offset and add new table to directory.
                offset++;
                pageKernelDirectory[(((uint32_t)&KERNEL_VIRTUAL_OFFSET - (dirIndex * PAGE_SIZE_1G)) / PAGE_SIZE_2M) + offset] =
                    (uint32_t)pageKernelTable | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
            }
            pageKernelTable[(page / PAGE_SIZE_4K) - (offset * 1024)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }

        // Create temporary directory, and map it for later use.
        pageKernelDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = (KERNEL_PAGE_TEMP - (uint32_t)&KERNEL_VIRTUAL_OFFSET) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

        // Enable PAE.
        asm volatile ("mov %cr4, %eax");
        asm volatile ("bts $5, %eax");
        asm volatile ("mov %eax, %cr4");

        // Enable paging.
	    asm volatile ("mov %%eax, %%cr3": :"a"((uint32_t)pageDirectoryPointerTable));	
	    asm volatile ("mov %cr0, %eax");
	    asm volatile ("orl $0x80000000, %eax");
	    asm volatile ("mov %eax, %cr0");
    }
    else { // Non-PAE paging mode.
        // Get pointer to page directory and clear it out.
        uint32_t *pageDirectory = (uint32_t*)(KERNEL_PAGE_TEMP - (uint32_t)&KERNEL_VIRTUAL_OFFSET);
        for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++)
            pageDirectory[i] = 0;

        // Create page table for low memory and init section of kernel.
        uint32_t *pageLowTable = (uint32_t*)ALIGN_4K(PAGE_FRAME_STACK_END - (uint32_t)&KERNEL_VIRTUAL_OFFSET);
        for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++)
            pageLowTable[i] = 0;

        // Identity map low memory and init section of kernel.
        for (page_t page = 0; page <= ((uint32_t)&KERNEL_INIT_END); page += PAGE_SIZE_4K)
            pageLowTable[page / PAGE_SIZE_4K] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        pageDirectory[0] = (uint32_t)pageLowTable | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

        // Create first kernel table and add it to directory.
        uint32_t *pageKernelTable = (uint32_t*)ALIGN_4K((uintptr_t)pageLowTable);
        pageDirectory[(uint32_t)&KERNEL_VIRTUAL_OFFSET / PAGE_SIZE_4M] =
            ((uint32_t)pageKernelTable) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

        // Map low memory and kernel to higher-half virtual space.
        uint32_t offset = 0;
        for (page_t page = 0; page <= (PAGE_FRAME_STACK_END - (uint32_t)&KERNEL_VIRTUAL_OFFSET); page += PAGE_SIZE_4K) {
            // Have we reached the need to create another table?
            if (page > 0 && page % PAGE_SIZE_4M == 0) {
                // Create a new table after the previous one.        
                uint32_t prevAddr = (uint32_t)pageKernelTable;
                pageKernelTable = (uint32_t*)ALIGN_4K(prevAddr);

                // Increase offset and add new table to directory.
                offset++;
                pageDirectory[(((uint32_t)&KERNEL_VIRTUAL_OFFSET) / PAGE_SIZE_4M) + offset] =
                    ((uint32_t)pageKernelTable) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
            }
            pageKernelTable[(page / PAGE_SIZE_4K) - (offset * PAGE_TABLE_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }

        // Set last entry to point to the new directory, this is used later.
        pageDirectory[PAGE_DIRECTORY_SIZE - 1] =
            (KERNEL_PAGE_DIRECTORY - (uint32_t)&KERNEL_VIRTUAL_OFFSET) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

        // Enable paging.
        asm volatile ("mov %%eax, %%cr3": :"a"((uint32_t)pageDirectory));	
        asm volatile ("mov %cr0, %eax");
        asm volatile ("orl $0x80000000, %eax");
        asm volatile ("mov %eax, %cr0");
    }
}
