/*
 * File: pmm.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
	uintptr_t dmaPageFrameFirst;
	uintptr_t dmaPageFrameLast;

#ifndef X86_64
	// Page frame stack (PAE).
    uintptr_t pageFrameStackPaeStart;
    uintptr_t pageFrameStackPaeEnd;
#endif

	// Page frame stack.
    uintptr_t pageFrameStackStart;
    uintptr_t pageFrameStackEnd;
	// * End of reserved kernel addresses *
};
typedef struct mem_info mem_info_t;
extern mem_info_t memInfo;

extern bool pmm_dma_get_free_frame(uintptr_t *frameOut);
extern void pmm_dma_set_frame(uintptr_t frame, bool status);
extern uintptr_t pmm_dma_get_phys(uintptr_t frame);
extern uintptr_t pmm_dma_get_virtual(uintptr_t frame);
extern uint32_t pmm_frames_available();
extern uint64_t pmm_pop_frame();
extern void pmm_push_frame(uint64_t frame);

#ifndef X86_64 // PAE does not apply to the 64-bit kernel.
extern uint32_t pmm_frames_available_pae();
extern uint32_t pmm_pop_frame_nonpae();
#endif


extern void pmm_init();

#endif
