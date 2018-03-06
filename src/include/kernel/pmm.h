#ifndef PMM_H
#define PMM_H

#include <main.h>
#include <multiboot.h>

#define PAGE_FRAME_STACK_SIZE		(4096*1024)
#define ALIGN_4K(size)          (((uint32_t)(size) + 0x1000) & 0xFFFFF000)

// Type for pages.
typedef uint32_t page_t;

struct mem_info {
	// Multiboot header.
	multiboot_info_t *mbootInfo;

	// Special kernel locations.
	uint32_t kernelVirtualOffset;
	uint32_t kernelStart;
	uint32_t kernelEnd;
	uint32_t mbootStart;
	uint32_t mbootEnd;

	// Memory info.
	uint32_t memoryKb;

	// Paging tables.
	uint32_t kernelPageDirectory;
	uint32_t KernelPageTemp; // Used for directly accessing a 4KB block of RAM.

	// Page frame stack.
    uint32_t pageFrameStackStart;
    uint32_t pageFrameStackEnd;
	// * End of reserved kernel addresses *
};
typedef struct mem_info mem_info_t;
extern mem_info_t memInfo;

extern uint32_t pmm_frames_available();
extern page_t pmm_pop_frame();
extern void pmm_push_frame(page_t frame);
extern void pmm_init(multiboot_info_t* mbootInfo);

#endif
