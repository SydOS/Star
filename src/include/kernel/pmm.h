#ifndef PAGE_H
#define PAGE_H

#include <main.h>
#include <multiboot.h>

// Page sizes.
#define PAGE_STACK_SIZE		(4096*1024)
#define PAGE_SIZE_4K        4096

// Type for pages.
typedef uint32_t page_t;

struct mem_info {
	multiboot_info_t *mbootInfo;
	multiboot_memory_map_t *mmap;
	uint32_t mmapLength;

	uint32_t kernelStart;
	uint32_t kernelEnd;
	uint32_t mbootStart;
	uint32_t mbootEnd;

	// Page stack.
    uint32_t pageStackStart;
    uint32_t pageStackEnd;

	// Memory info.
	uint32_t memoryKb;
};
typedef struct mem_info mem_info_t;
extern mem_info_t memInfo;

// Macros for checking and aligning.
#define ALIGN_4K(size)          (((uint32_t)(size) + 0x1000) & 0xFFFFF000)
#define MASK_PAGE_4K(size)      ((uint32_t)(size) & 0xFFFFF000)
#define MASK_PAGEFLAGS_4K(size)      ((uint32_t)(size) & ~0xFFFFF000)

extern mem_info_t memInfo;
extern void pmm_push_page(page_t page);
extern page_t pmm_pop_page();
extern void pmm_init(multiboot_info_t* mbootInfo);

#endif
