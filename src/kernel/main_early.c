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
uint32_t EARLY_PAGES_LAST __attribute__((section(".inittables")));
bool PAE_ENABLED __attribute__((section(".inittables"))) = false;

extern uint32_t _cpuid_detect_early();

__attribute__((section(".init"))) static void memset(void *str, int32_t c, size_t n) {
	uint8_t *s = (uint8_t*)str;

	// Copy byte.
	for (size_t i = 0; i < n; i++)
		s[i] = (uint8_t)c;
}

__attribute__((section(".init"))) static void setup_paging() {
    // Get kernel virtual offset.
    uint32_t virtualOffset = (uint32_t)&KERNEL_VIRTUAL_OFFSET;

    // Create page directory.
    uint32_t *pageDirectory = (uint32_t*)ALIGN_4K(PAGE_FRAME_STACK_END - virtualOffset);
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create page table for mapping low memory + ".init" section of kernel. This is an identity mapping.
    uint32_t *pageTableLow = (uint32_t*)ALIGN_4K((uint32_t)pageDirectory);
    memset(pageTableLow, 0, PAGE_SIZE_4K);

    // Identity map low memory + ".init" section of kernel.
    for (uint32_t page = 0; page <= (uint32_t)&KERNEL_INIT_END; page += PAGE_SIZE_4K)
        pageTableLow[page / PAGE_SIZE_4K] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    pageDirectory[0] = (uint32_t)pageTableLow | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Create first kernel table and add it to directory.
    uint32_t *pageTableKernel = (uint32_t*)ALIGN_4K((uint32_t)pageTableLow);
    memset(pageTableKernel, 0, PAGE_SIZE_4K);
    pageDirectory[virtualOffset / PAGE_SIZE_4M] = ((uint32_t)pageTableKernel) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (page_t page = 0; page <= PAGE_FRAME_STACK_END - virtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_4M == 0) {
            // Create a new table after the previous one.        
            uint32_t prevAddr = (uint32_t)pageTableKernel;
            pageTableKernel = (uint32_t*)ALIGN_4K(prevAddr);
            memset(pageTableKernel, 0, PAGE_SIZE_4K);

            // Increase offset and add new table to directory.
            offset++;
            pageDirectory[(virtualOffset / PAGE_SIZE_4M) + offset] = ((uint32_t)pageTableKernel) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }

        // Add page to table.
        pageTableKernel[(page / PAGE_SIZE_4K) - (offset * PAGE_TABLE_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Set end of temporary reserved pages.
    EARLY_PAGES_LAST = (uint32_t)pageTableKernel;

    // Set last entry to point to the new directory, this is used later.
    pageDirectory[PAGE_DIRECTORY_SIZE - 1] = (uint32_t)pageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Enable 32-bit paging.
    asm volatile ("mov %%eax, %%cr3" : : "a"((uint32_t)pageDirectory));	
    asm volatile ("mov %cr0, %eax");
    asm volatile ("orl $0x80000000, %eax");
    asm volatile ("mov %eax, %cr0");
}

__attribute__((section(".init"))) static void setup_pae_paging() {
    // Get kernel virtual offset.
    uint32_t virtualOffset = (uint32_t)&KERNEL_VIRTUAL_OFFSET;

    // Create page directory pointer table (PDPT).
    uint64_t *pageDirectoryPointerTable = (uint64_t*)ALIGN_4K(PAGE_FRAME_STACK_END - virtualOffset);
    memset(pageDirectoryPointerTable, 0, PAGE_SIZE_4K);

    // Create page directory for the 0GB space.
    uint64_t *pageDirectoryLow = (uint64_t*)ALIGN_4K((uint32_t)pageDirectoryPointerTable);
    memset(pageDirectoryLow, 0, PAGE_SIZE_4K);
    pageDirectoryPointerTable[0] = (uint64_t)pageDirectoryLow | PAGING_PAGE_PRESENT;

    // Create page table for mapping low memory + ".init" section of kernel. This is an identity mapping.
    uint64_t *pageTableLow = (uint64_t*)ALIGN_4K((uint32_t)pageDirectoryLow);
    memset(pageTableLow, 0, PAGE_SIZE_4K);

    // Identity map low memory + ".init" section of kernel.
    for (uint32_t page = 0; page <= (uint32_t)&KERNEL_INIT_END; page += PAGE_SIZE_4K)
        pageTableLow[page / PAGE_SIZE_4K] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    pageDirectoryLow[0] = (uint64_t)pageTableLow | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Create our kernel page directory (0xC0000000 to 0xFFFFFFFF).
    uint64_t *pageDirectoryKernel = (uint64_t*)ALIGN_4K((uint32_t)pageTableLow);
    memset(pageDirectoryKernel, 0, PAGE_SIZE_4K);
    pageDirectoryPointerTable[3] = (uint64_t)pageDirectoryKernel | PAGING_PAGE_PRESENT;

    // Create first table and add it to directory.
    uint64_t *pageTableKernel = (uint64_t*)ALIGN_4K((uint32_t)pageDirectoryKernel);
    memset(pageTableKernel, 0, PAGE_SIZE_4K);
    pageDirectoryKernel[0] = (uint64_t)pageTableKernel | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint64_t page = 0; page <= PAGE_FRAME_STACK_END - virtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_2M == 0) {
            // Create a new table after the previous one.        
            uint32_t prevAddr = (uint32_t)pageTableKernel;
            pageTableKernel = (uint64_t*)ALIGN_4K(prevAddr);
            memset(pageTableKernel, 0, PAGE_SIZE_4K);

            // Increase offset and add new table to directory.
            offset++;
            pageDirectoryKernel[offset] = (uint64_t)pageTableKernel | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }
        pageTableKernel[(page / PAGE_SIZE_4K) - (offset * 1024)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Set end of temporary reserved pages.
    EARLY_PAGES_LAST = (uint32_t)pageTableKernel;

    // Enable PAE.
    asm volatile ("mov %cr4, %eax");
    asm volatile ("bts $5, %eax");
    asm volatile ("mov %eax, %cr4");

    // Enable PAE paging.
    asm volatile ("mov %%eax, %%cr3" : : "a"((uint32_t)pageDirectoryPointerTable));	
    asm volatile ("mov %cr0, %eax");
    asm volatile ("orl $0x80000000, %eax");
    asm volatile ("mov %eax, %cr0");
}

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
    //PAE_ENABLED = false;

    // Count up memory in bytes.
    uint64_t memory = 0;
    uintptr_t base = mbootInfo->mmap_addr;
	uintptr_t end = base + mbootInfo->mmap_length;
	for(multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base; base < end; base += entry->size + sizeof(uintptr_t)) {
		entry = (multiboot_memory_map_t*)base;
		if(entry->type == 1 && ((uint32_t)entry->addr) > 0)
			memory += entry->len;
	}

    // Determine stack offset and size.
    PAGE_FRAME_STACK_START = ALIGN_4K((uint32_t)&KERNEL_VIRTUAL_END);
    PAGE_FRAME_STACK_END = PAGE_FRAME_STACK_START + ((memory / PAGE_SIZE_4K) * sizeof(uint32_t)); // Space for 32-bit addresses.

    // Is PAE enabled?
    if (PAE_ENABLED)
        setup_pae_paging(); // Set up PAE paging.
    else
        setup_paging(); // Set up non-PAE paging.
}
