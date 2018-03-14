#ifndef PMM_H
#define PMM_H

#include <main.h>
#include <multiboot.h>

// Type for page frames.
typedef uintptr_t page_t;

#define PMM_NO_OF_DMA_FRAMES	64

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
	page_t kernelPageDirectory;
	bool paeEnabled;
	bool nxEnabled;

	// DMA frames.
	uint32_t dmaPageFrameFirst;
	uint32_t dmaPageFrameLast;

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
