#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <kernel/memory/paging.h>

#include <kernel/interrupts/exceptions.h>
#include <kernel/memory/pmm.h>

// http://www.rohitab.com/discuss/topic/31139-tutorial-paging-memory-mapping-with-a-recursive-page-directory/
// https://forum.osdev.org/viewtopic.php?f=15&t=19387
// https://medium.com/@connorstack/recursive-page-tables-ad1e03b20a85

#ifdef X86_64
extern void paging_late_long();
#else
extern void paging_late_std();
extern void paging_late_pae();
#endif

/**
 * Changes the current paging structure.
 * @param directoryPhysicalAddr The physical address of the root paging structure.
 */
void paging_change_directory(uintptr_t directoryPhysicalAddr) {
    // Tell CPU the directory and enable paging.
    asm volatile ("mov %%eax, %%cr3": :"a"(directoryPhysicalAddr)); 
    asm volatile ("mov %cr0, %eax");
    asm volatile ("orl $0x80000000, %eax");
    asm volatile ("mov %eax, %cr0");
}

/**
 * Flushes the TLB.
 */
void paging_flush_tlb() {
    // Flush TLB.
    asm volatile ("mov %cr3, %eax");
    asm volatile ("mov %eax, %cr3");
}

/**
 * Flushes a specific address from the TLB.
 * @param address The address to flush.
 */
void paging_flush_tlb_address(uintptr_t address) {
#ifdef I486
    // Flush specified address in TLB.
    asm volatile ("invlpg (%0)" : : "b"(address) : "memory");
#else
    // 386 and below don't have the invlpg instruction.
    paging_flush_tlb();
#endif
}

/**
 * Maps a region of virtual memory.
 * @param startAddress The first address to map.
 * @param endAddress The last address to map.
 * @param kernel Is the region for the kernel?
 * @param writeable Is the region read/write?
 */
void paging_map_region(uintptr_t startAddress, uintptr_t endAddress, bool kernel, bool writeable) {
    // Ensure addresses are on 4KB boundaries.
    if (MASK_PAGEFLAGS_4K(startAddress) || MASK_PAGEFLAGS_4K(endAddress))
        panic("PAGING: Non-4KB aligned address range (0x%p-0x%p) specified!\n", startAddress, endAddress);
    if (startAddress > endAddress)
        panic("PAGING: Start address (0x%p) is after end address (0x%p)!\n", startAddress, endAddress);

    // Map range, popping physical page frames for each virtual page.
    for (uint32_t i = 0; i <= (endAddress - startAddress) / PAGE_SIZE_4K; i++)
        paging_map(startAddress + (i * PAGE_SIZE_4K), pmm_pop_frame(), kernel, writeable);
}

/**
 * Maps a region of virtual memory using the specified physical address.
 * @param startAddress The first address to map.
 * @param endAddress The last address to map.
 * @param startPhys The first physical address to map to.
 * @param kernel Is the region for the kernel?
 * @param writeable Is the region read/write?
 */
void paging_map_region_phys(uintptr_t startAddress, uintptr_t endAddress, uint64_t startPhys, bool kernel, bool writeable) {
    // Ensure addresses are on 4KB boundaries.
    if (MASK_PAGEFLAGS_4K(startAddress) || MASK_PAGEFLAGS_4K(endAddress))
        panic("PAGING: Non-4KB aligned address range (0x%p-0x%p) specified!\n", startAddress, endAddress);
    if (startAddress > endAddress)
        panic("PAGING: Start address (0x%p) is after end address (0x%p)!\n", startAddress, endAddress);
    if (MASK_PAGEFLAGS_4K_64BIT(startPhys))
        panic("PAGING: Non-4KB aligned physical start address (0x%llX) specified!\n", startPhys);

    // Map space, starting with the physical address specified.
    for (uint32_t i = 0; i <= (endAddress - startAddress) / PAGE_SIZE_4K; i++)
        paging_map(startAddress + (i * PAGE_SIZE_4K), startPhys + (i * PAGE_SIZE_4K), kernel, writeable);
}

/**
 * Unmaps a region of virtual memory.
 * @param startAddress The first address to unmap.
 * @param endAddress The last address to unmap.
 */
void paging_unmap_region(uintptr_t startAddress, uintptr_t endAddress) {
    // Ensure addresses are on 4KB boundaries.
    if (MASK_PAGEFLAGS_4K(startAddress) || MASK_PAGEFLAGS_4K(endAddress))
        panic("PAGING: Non-4KB aligned address range (0x%p-0x%p) specified!\n", startAddress, endAddress);
    if (startAddress > endAddress)
        panic("PAGING: Start address (0x%p) is after end address (0x%p)!\n", startAddress, endAddress);

    // Unmap range, freeing page frames.
    for (uint32_t i = 0; i <= (endAddress - startAddress) / PAGE_SIZE_4K; i++) {
        uintptr_t frame = 0;
        bool mapped = paging_get_phys(startAddress + (i * PAGE_SIZE_4K), &frame);
        paging_unmap(startAddress + (i * PAGE_SIZE_4K));

        // Push frame if needed.
        if (mapped)
            pmm_push_frame(frame);
    }
}

void *paging_device_alloc(uint64_t startPhys, uint64_t endPhys) {
    // Ensure addresses are on 4KB boundaries.
    if (MASK_PAGEFLAGS_4K_64BIT(startPhys) || MASK_PAGEFLAGS_4K_64BIT(endPhys))
        panic("PAGING: Non-4KB aligned address range (0x%llX-0x%llX) specified!\n", startPhys, endPhys);
    if (startPhys > endPhys)
        panic("PAGING: Start address (0x%llX) is after end address (0x%llX)!\n", startPhys, endPhys);

    uint32_t pageCount = ((endPhys - startPhys) / PAGE_SIZE_4K) + 1;// DIVIDE_ROUND_UP(MASK_PAGEFLAGS_4K(PhysicalAddress) + Length, PAGE_SIZE_4K);

    // Get next available virtual range.
    uintptr_t page = PAGING_FIRST_DEVICE_ADDRESS;
    uint32_t currentCount = 0;
    uint64_t phys;
    while (currentCount < pageCount) {     
        // Check if page is free.
        if (!paging_get_phys(page, &phys))
            currentCount++;
        else {
            // Move to next page.
            page += PAGE_SIZE_4K;
            if (page > PAGING_LAST_DEVICE_ADDRESS)
                panic("PAGING: Out of device virtual addresses!\n");

            // Start back at zero.
            currentCount = 0;
        }
    }

    // Check if last page is outside bounds.
    if (page + ((pageCount - 1) * PAGE_SIZE_4K) > PAGING_LAST_DEVICE_ADDRESS)
        panic("PAGING: Out of device virtual addresses!\n");

    // Map range.
    paging_map_region_phys(page, page + ((pageCount - 1) * PAGE_SIZE_4K), startPhys, true, true);

    // Return address.
    return (void*)(page);
}

/**
 * Unmaps a region of virtual memory without returning pages to the page stack, or unmapping non-stack memory.
 * @param startAddress The first address to unmap.
 * @param endAddress The last address to unmap.
 */
void paging_unmap_region_phys(uintptr_t startAddress, uintptr_t endAddress) {
    // Ensure addresses are on 4KB boundaries.
    if (MASK_PAGEFLAGS_4K(startAddress) || MASK_PAGEFLAGS_4K(endAddress))
        panic("PAGING: Non-4KB aligned address range (0x%p-0x%p) specified!\n", startAddress, endAddress);
    if (startAddress > endAddress)
        panic("PAGING: Start address (0x%p) is after end address (0x%p)!\n", startAddress, endAddress);

    // Unmap range.
    for (uint32_t i = 0; i <= (endAddress - startAddress) / PAGE_SIZE_4K; i++)
        paging_unmap(startAddress + (i * PAGE_SIZE_4K));
}

void paging_device_free(uintptr_t startAddress, uintptr_t endAddress) {
    paging_unmap_region_phys(startAddress, endAddress);
}

static void paging_pagefault_handler(ExceptionRegisters_t *regs) {
    page_t addr;
    asm volatile ("mov %%cr2, %0" : "=r"(addr));
/*#ifdef X86_64
    kprintf("RAX: 0x%p, RBX: 0x%p, RCX: 0x%p, RDX: 0x%p\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    kprintf("RSI: 0x%p, RDI: 0x%p, RBP: 0x%p, RSP: 0x%p\n", regs->rsi, regs->rdi, regs->rbp, regs->rsp);
    kprintf("RIP: 0x%p, RFLAGS: 0x%p\n", regs->rip, regs->rflags);
#else
    kprintf("EAX: 0x%p, EBX: 0x%p, ECX: 0x%p, EDX: 0x%p\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("ESI: 0x%p, EDI: 0x%p, EBP: 0x%p, ESP: 0x%p\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    kprintf("EIP: 0x%p, EFLAGS: 0x%p\n", regs->eip, regs->eflags);
#endif*/
    kprintf("EAX: 0x%p, EBX: 0x%p, ECX: 0x%p, EDX: 0x%p\n", regs->ax, regs->bx, regs->cx, regs->dx);
    kprintf("ESI: 0x%p, EDI: 0x%p, EBP: 0x%p, ESP: 0x%p\n", regs->si, regs->di, regs->bp, regs->sp);
    kprintf("EIP: 0x%p, EFLAGS: 0x%p\n", regs->ip, regs->flags);
    panic("PAGING: Page fault at 0x%X!\n", addr);
}

/**
 * Initializes paging.
 */
void paging_init() {
    kprintf("PAGING: Initializing...\n");

    // Wire up page fault handler.
    exceptions_install_handler(EXCEPTION_PAGE_FAULT, paging_pagefault_handler);

#ifdef X86_64
    // Setup 4-level (long mode) paging.
    paging_late_long();
#else
    // Is PAE enabled?
    if (memInfo.paeEnabled)
        paging_late_pae(); // Use PAE paging.
    else 
        paging_late_std(); // No PAE, using standard paging.
#endif
        
    // Change to use our new page directory.
    paging_change_directory(memInfo.kernelPageDirectory);
    
    // Map range from 0x1000 to 0x5000 for testing.
    kprintf("PAGING: Mapping range 0x1000 to 0x5000...\n");
    paging_map_region(0x1000, 0x5000, true, true);

    // Test memory at location.
    kprintf("PAGING: Testing memory at virtual address 0x1000...\n");
    uint32_t *testPage = (uint32_t*)0x1000;
    for (uint32_t i = 0; i < (PAGE_SIZE_4K / sizeof(uint32_t)) * 4; i++)
        testPage[i] = i;

    bool pass = true;
    for (uint32_t i = 0; i < (PAGE_SIZE_4K / sizeof(uint32_t)) * 4; i++)
        if (testPage[i] != i) {
            pass = false;
            break;
        }
    kprintf("PAGING: Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("PAGING: Test failed.\n");

    // Unmap virtual address and return page to stack.
    kprintf("PAGING: Unmapping test region...\n");
    paging_unmap_region(0x1000, 0x5000);

    kprintf("PAGING: Initialized!\n");
}
