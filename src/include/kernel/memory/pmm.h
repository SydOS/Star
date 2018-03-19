#ifndef PMM_H
#define PMM_H

#include <main.h>
#ifdef X86_64
#define PMM_MULTIBOOT2
#include <multiboot2.h>
#else
#include <multiboot.h>
#endif

// Type for page frames.
typedef uintptr_t page_t;

#define PMM_NO_OF_DMA_FRAMES	64

struct mem_info {
	// Multiboot header.
	multiboot_info_t *mbootInfo;

	// Special kernel locations.
	uintptr_t kernelVirtualOffset;
	uintptr_t kernelStart;
	uintptr_t kernelEnd;
	uintptr_t mbootStart;
	uintptr_t mbootEnd;

	// Memory info.
	uint32_t memoryKb;

	// Paging tables.
	uintptr_t kernelPageDirectory;
	bool paeEnabled;
	bool nxEnabled;

	// DMA frames.
	uint32_t dmaPageFrameFirst;
	uint32_t dmaPageFrameLast;

#ifndef X86_64
	// Page frame stack (PAE).
    uint32_t pageFrameStackPaeStart;
    uint32_t pageFrameStackPaeEnd;
#endif

	// Page frame stack.
    uintptr_t pageFrameStackStart;
    uintptr_t pageFrameStackEnd;
	// * End of reserved kernel addresses *
};
typedef struct mem_info mem_info_t;
extern mem_info_t memInfo;

extern bool pmm_dma_get_free_frame(uint32_t *frameOut);
extern void pmm_dma_set_frame(uint32_t frame, bool status);
extern uint32_t pmm_dma_get_phys(uint32_t frame);
extern uint32_t pmm_frames_available();
extern uintptr_t pmm_pop_frame();
extern void pmm_push_frame(uintptr_t frame);

#ifndef X86_64 // PAE does not apply to the 64-bit kernel.
extern uint32_t pmm_frames_available_pae();
extern uint64_t pmm_pop_frame_pae();
extern void pmm_push_frame_pae(uint64_t frame);
#endif


extern void pmm_init();

#endif
