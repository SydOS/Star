#include <main.h>
#include <kprint.h>
#include <string.h>
#include <multiboot.h>

#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>

// Constants determined by linker and early boot.
extern uint32_t KERNEL_VIRTUAL_OFFSET;
extern uint32_t KERNEL_VIRTUAL_START;
extern uint32_t KERNEL_VIRTUAL_END;

extern uint32_t DMA_FRAMES_FIRST;
extern uint32_t DMA_FRAMES_LAST;
extern uint32_t PAGE_FRAME_STACK_PAE_START;
extern uint32_t PAGE_FRAME_STACK_PAE_END;
extern uint32_t PAGE_FRAME_STACK_START;
extern uint32_t PAGE_FRAME_STACK_END;
extern uint32_t EARLY_PAGES_LAST;
extern bool PAE_ENABLED;

// Used to store info about memory in the system.
mem_info_t memInfo;
uint32_t earlyPagesLast;

// DMA bitmap. Each bit represents a 64KB page, in order.
static bool dmaFrames[PMM_NO_OF_DMA_FRAMES];

// Page frame stack, stores addresses to 4K page frames in physical memory. This stack contains only PAE pages.
static uint64_t *pageFrameStackPae;
static uint32_t pageFramesPaeAvailable = 0;

// Page frame stack, stores addresses to 4K page frames in physical memory.
static page_t *pageFrameStack;
static uint32_t pageFramesAvailable = 0;

/**
 * 
 * DMA MEMORY FUNCTIONS
 * 
 */

/**
 * Gets a free DMA frame.
 * @param frameOut Pointer to where the frame address should be stored.
 * @return True if the function succeeded; otherwise false.
 */
bool pmm_dma_get_free_frame(uint32_t *frameOut) {
    // Search until we find a free page frame.
    for (uint32_t frame = 0; frame < PMM_NO_OF_DMA_FRAMES; frame++) {
        if (dmaFrames[frame]) {
            // We found a frame.
            *frameOut = memInfo.dmaPageFrameFirst + (frame * PAGE_SIZE_64K);
            dmaFrames[frame] = false;
            return true;
        }
    }

    // No frame found.
    return false;
}

void pmm_dma_set_frame(uint32_t frame, bool status) {
    // Ensure we are in bounds and aligned.
    if (frame < memInfo.dmaPageFrameFirst || frame > memInfo.dmaPageFrameLast || (frame % PAGE_SIZE_64K != 0))
        panic("PMM: Invalid DMA frame 0x%X specified!\n", frame);

    // Change status of frame.
    dmaFrames[(memInfo.dmaPageFrameFirst - frame) / PAGE_SIZE_64K] = status;
}

uint32_t pmm_dma_get_phys(uint32_t frame) {
    return frame - memInfo.kernelVirtualOffset;
}

/**
 * Builds the DMA bitmap.
 */
static void pmm_dma_build_bitmap() {
    // Zero out frames and set frame available.
    for (uint32_t frame = 0; frame < PMM_NO_OF_DMA_FRAMES; frame++) {
        memset((void*)memInfo.dmaPageFrameFirst + (frame * PAGE_SIZE_64K), 0, PAGE_SIZE_64K);
        dmaFrames[frame] = true;
    }

    // Test out frame functions.
    kprintf("PMM: Testing DMA memory manager...\n");
    uint32_t frame1;
    if (!pmm_dma_get_free_frame(&frame1))
        panic("PMM: Couldn't get DMA frame!\n");

    // Test memory.
    kprintf("PMM: Testing %uKB of memory at 0x%X (0x%X)...", PAGE_SIZE_64K / 1024, frame1, pmm_dma_get_phys(frame1));
    uint32_t *framePtr = (uint32_t*)frame1;
    for (uint32_t i = 0; i < PAGE_SIZE_64K / sizeof(uint32_t); i++)
        framePtr[i] = i;

    bool pass = true;
    for (uint32_t i = 0; i < PAGE_SIZE_64K / sizeof(uint32_t); i++)
        if (framePtr[i] != i) {
            pass = false;
            break;
        }
    kprintf("%s!\n", pass ? "passed" : "failed");

    // Pull another frame.
    uint32_t frame2;
    if (!pmm_dma_get_free_frame(&frame2))
        panic("PMM: Couldn't get DMA frame!\n");

    // Test memory.
    kprintf("PMM: Testing %uKB of memory at 0x%X (0x%X)...", PAGE_SIZE_64K / 1024, frame2, pmm_dma_get_phys(frame2));
    framePtr = (uint32_t*)frame2;
    for (uint32_t i = 0; i < PAGE_SIZE_64K / sizeof(uint32_t); i++)
        framePtr[i] = i;

    pass = true;
    for (uint32_t i = 0; i < PAGE_SIZE_64K / sizeof(uint32_t); i++)
        if (framePtr[i] != i) {
            pass = false;
            break;
        }
    kprintf("%s!\n", pass ? "passed" : "failed");

    // Free both frames.
    pmm_dma_set_frame(frame1, true);
    pmm_dma_set_frame(frame2, true);
    kprintf("PMM: DMA memory manager test complete.\n");
}

/**
 * 
 * NORMAL MEMORY FUNCTIONS
 * 
 */

/**
 * Gets the current number of page frames available.
 */
uint32_t pmm_frames_available() {
    return pageFramesAvailable;
}

/**
 * Gets the current number of PAE page frames available.
 */
uint32_t pmm_frames_available_pae() {
    return pageFramesPaeAvailable;
}

/**
 * Pops a page frame off the stack.
 * @return 		The physical address of the page frame.
 */
page_t pmm_pop_frame() {
    // Get frame off stack.
    page_t frame = *pageFrameStack;

    // Verify there are frames.
    if (pmm_frames_available() == 0)
        panic("PMM: No more page frames!\n");

    // Decrement stack and return frame.
    pageFrameStack--;
    pageFramesAvailable--;
    return frame;
}

/**
 * Pops a page frame off the PAE stack.
 * @return 		The physical address of the page frame.
 */
uint64_t pmm_pop_frame_pae() {
    // Get frame off stack.
    uint64_t frame = *pageFrameStackPae;

    // Verify there are frames.
    if (pmm_frames_available() == 0)
        panic("PMM: No more PAE page frames!\n");

    // Decrement stack and return frame.
    pageFrameStackPae--;
    pageFramesPaeAvailable--;
    return frame;
}

/**
 * Pushes a page frame to the stack.
 * @param frame	The physical address of the page frame to push.
 */
void pmm_push_frame(page_t frame) {
    // Increment stack pointer and check its within bounds.
    pageFrameStack++;
    if (((page_t)pageFrameStack) < memInfo.pageFrameStackStart || ((page_t)pageFrameStack) >= memInfo.pageFrameStackEnd)
        panic("PMM: Page frame stack pointer out of bounds!\n");

    // Push frame to stack.
    *pageFrameStack = frame;
    pageFramesAvailable++;
}

/**
 * Pushes a page frame to the PAE stack.
 * @param frame	The physical address of the page frame to push.
 */
void pmm_push_frame_pae(uint64_t frame) {
    // Increment stack pointer and check its within bounds.
    pageFrameStackPae++;
    if (((uint32_t)pageFrameStackPae) < memInfo.pageFrameStackPaeStart || ((uint32_t)pageFrameStackPae) >= memInfo.pageFrameStackPaeEnd)
        panic("PMM: PAE page frame stack pointer out of bounds!\n");

    // Push frame to stack.
    *pageFrameStackPae = frame;
    pageFramesPaeAvailable++;
}

/**
 * Prints the memory map.
 */
void pmm_print_memmap() {
    kprintf("PMM: Physical memory map:\n");
    uint64_t memory = 0;

    uint32_t base = memInfo.mbootInfo->mmap_addr;
    uint32_t end = base + memInfo.mbootInfo->mmap_length;
    multiboot_memory_map_t* entry = (multiboot_memory_map_t*)base;

    for (; base < end; base += entry->size + sizeof(uint32_t)) {
        entry = (multiboot_memory_map_t*)base;

        // Print out info.
        kprintf("PMM:     region start: 0x%llX length: 0x%llX type: 0x%X\n", entry->addr, entry->len, (uint64_t)entry->type);
        if (entry->type == 1 && (((uint32_t)entry->addr) > 0 || (memInfo.paeEnabled && (entry->addr & 0xF00000000))))
            memory += entry->len;
    }

    // Print summary.
    kprintf("PMM: Kernel start: 0x%X | Kernel end: 0x%X\n", memInfo.kernelStart, memInfo.kernelEnd);
    kprintf("PMM: Multiboot info start: 0x%X | Multiboot info end: 0x%X\n", memInfo.mbootStart, memInfo.mbootEnd);
    kprintf("PMM: Page frame stack start: 0x%X | Page stack end: 0x%X\n", memInfo.pageFrameStackStart, memInfo.pageFrameStackEnd);
    if (memInfo.paeEnabled && memInfo.pageFrameStackPaeStart > 0 && memInfo.pageFrameStackPaeEnd > 0)
        kprintf("PMM: PAE page frame stack start: 0x%X | Page stack end: 0x%X\n", memInfo.pageFrameStackPaeStart, memInfo.pageFrameStackPaeEnd);

    memInfo.memoryKb = memory / 1024;
    kprintf("PMM: Detected usable RAM: %uKB\n", memInfo.memoryKb);
}

/**
 * Builds the page frame stacks.
 */
static void pmm_build_stacks() {
    // Initialize stack.
    kprintf("PMM: Initializing page frame stack at 0x%X...\n", memInfo.pageFrameStackStart);
    pageFrameStack = (page_t*)(memInfo.pageFrameStackStart);
    memset(pageFrameStack, 0, memInfo.pageFrameStackEnd - memInfo.pageFrameStackStart);

    // Perform memory test on stack areas.
    kprintf("PMM: Testing %uKB of memory at 0x%X...", (memInfo.pageFrameStackEnd - memInfo.pageFrameStackStart) / 1024, memInfo.pageFrameStackStart);
    for (page_t i = 0; i <= (memInfo.pageFrameStackEnd - memInfo.pageFrameStackStart) / sizeof(page_t); i++)
        pageFrameStack[i] = i;

    bool pass = true;
    for (page_t i = 0; i <= (memInfo.pageFrameStackEnd - memInfo.pageFrameStackStart) / sizeof(page_t); i++)
        if (pageFrameStack[i] != i) {
            pass = false;
            break;
        }
    kprintf("%s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("PMM: Memory test of page frame stack area failed.\n");

    // If PAE is enabled, initialize PAE stack.
    if (memInfo.paeEnabled && memInfo.pageFrameStackPaeStart > 0 && memInfo.pageFrameStackPaeEnd > 0) {
        // Initialize stack pointer.
        kprintf("PMM: Initializing PAE page frame stack at 0x%X...\n", memInfo.pageFrameStackPaeStart);
        pageFrameStackPae = (uint64_t*)(memInfo.pageFrameStackPaeStart);
        memset(pageFrameStackPae, 0, memInfo.pageFrameStackPaeEnd - memInfo.pageFrameStackPaeStart);

        // Perform memory test on stack areas.
        kprintf("PMM: Testing %uKB of memory at 0x%X...", (memInfo.pageFrameStackPaeEnd - memInfo.pageFrameStackPaeStart) / 1024, memInfo.pageFrameStackPaeStart);
        for (uint64_t i = 0; i <= (memInfo.pageFrameStackPaeEnd - memInfo.pageFrameStackPaeStart) / sizeof(uint64_t); i++)
            pageFrameStackPae[i] = i;

        bool pass = true;
        for (uint64_t i = 0; i <= (memInfo.pageFrameStackPaeEnd - memInfo.pageFrameStackPaeStart) / sizeof(uint64_t); i++)
            if (pageFrameStackPae[i] != i) {
                pass = false;
                break;
            }
        kprintf("%s!\n", pass ? "passed" : "failed");
        if (!pass)
            panic("PMM: Memory test of PAE page frame stack area failed.\n");
    }

    // Build stack of free page frames.
    for (multiboot_memory_map_t *entry = (multiboot_memory_map_t*)memInfo.mbootInfo->mmap_addr; (uint32_t)entry < memInfo.mbootInfo->mmap_addr + memInfo.mbootInfo->mmap_length;
        entry = (multiboot_memory_map_t*)((uint32_t)entry + entry->size + sizeof(entry->size))) {
        
        // If not available memory, skip over.
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;

        // If the entry is normal memory, add it to main stack.
        if (((page_t)entry->addr) > 0) {
            // Add frame to stack.
            uint32_t pageFrameBase = ALIGN_4K(entry->addr);	
            kprintf("PMM: Adding pages in 0x%X!\n", pageFrameBase);			
            for (uint32_t i = 0; i < (entry->len / PAGE_SIZE_4K) - 1; i++) { // Give buffer incase another section of the memory map starts partway through a page.
                uint32_t addr = pageFrameBase + (i * PAGE_SIZE_4K);

                // If the address is in conventional memory (low memory), or is reserved by
                // the kernel or the frame stack, don't mark it free.
                if (addr <= 0x100000 || (addr >= (memInfo.kernelStart - memInfo.kernelVirtualOffset) &&
                    addr <= (memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset)) || addr >= entry->addr + entry->len)
                    continue;

                // Add frame to stack.
                pmm_push_frame(addr);
            }
        }
        // Is the entry a 64-bit one and PAE is enabled?
        else if (memInfo.paeEnabled && (entry->addr & 0xF00000000)) {
            // Add frame to stack.
            uint64_t pageFrameBase = ALIGN_4K_64BIT(entry->addr);	
            kprintf("PMM: Adding PAE pages in 0x%llX!\n", pageFrameBase);			
            for (uint32_t i = 0; i < (entry->len / PAGE_SIZE_4K) - 1; i++) { // Give buffer incase another section of the memory map starts partway through a page.
                uint64_t addr = pageFrameBase + (i * PAGE_SIZE_4K);

                // If the address is in conventional memory (low memory), or is reserved by
                // the kernel or the frame stack, don't mark it free.
                if (addr <= 0x100000 || (addr >= (memInfo.kernelStart - memInfo.kernelVirtualOffset) &&
                    addr <= (memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset)) || addr >= entry->addr + entry->len)
                    continue;

                // Add frame to stack.
                pmm_push_frame_pae(addr);
            }
        }
    }

    // Print out status.
    kprintf("PMM: Added %u page frames!\n", pageFramesAvailable);
    kprintf("PMM: First page on stack: 0x%X\n", *pageFrameStack);
    if (memInfo.paeEnabled && pageFramesAvailable > 0) {
        kprintf("PMM: Added %u PAE page frames!\n", pageFramesPaeAvailable);
        kprintf("PMM: First page on PAE stack: 0x%llX\n", *pageFrameStackPae);
    }
}

/**
 * Initializes the physical memory manager.
 */
void pmm_init(multiboot_info_t *mbootInfo) {
    kprintf("PMM: Initializing physical memory manager...\n");

    // Store away Multiboot info.
    memInfo.mbootInfo = (multiboot_info_t *)((uint32_t)mbootInfo + (uint32_t)&KERNEL_VIRTUAL_OFFSET);

    // Store where the Multiboot info structure actually resides in memory.
    memInfo.mbootStart = (uint32_t)mbootInfo;
    memInfo.mbootEnd = (uint32_t)(mbootInfo + sizeof(multiboot_info_t));

    // Store where the kernel is. These come from the linker.
    memInfo.kernelVirtualOffset = (uint32_t)&KERNEL_VIRTUAL_OFFSET;
    memInfo.kernelStart = (uint32_t)&KERNEL_VIRTUAL_START;
    memInfo.kernelEnd = (uint32_t)&KERNEL_VIRTUAL_END;

    // Store where the DMA page frames are.
    memInfo.dmaPageFrameFirst = DMA_FRAMES_FIRST;
    memInfo.dmaPageFrameLast = DMA_FRAMES_LAST;

    // Store page frame stack locations. This is determined during early boot in kernel_main_early().
    memInfo.pageFrameStackPaeStart = PAGE_FRAME_STACK_PAE_START;
    memInfo.pageFrameStackPaeEnd = PAGE_FRAME_STACK_PAE_END;
    memInfo.pageFrameStackStart = PAGE_FRAME_STACK_START;
    memInfo.pageFrameStackEnd = PAGE_FRAME_STACK_END;
    memInfo.paeEnabled = PAE_ENABLED;
    earlyPagesLast = EARLY_PAGES_LAST;

    // Print memory map.
    pmm_print_memmap();

    // Build DMA bitmap.
    pmm_dma_build_bitmap();

    // Build stacks.
    pmm_build_stacks();
    kprintf("PMM: Initialized!\n");
}
