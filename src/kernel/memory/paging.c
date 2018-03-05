#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <arch/i386/kernel/interrupts.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>

// http://www.rohitab.com/discuss/topic/31139-tutorial-paging-memory-mapping-with-a-recursive-page-directory/
// https://forum.osdev.org/viewtopic.php?f=15&t=19387
// https://medium.com/@connorstack/recursive-page-tables-ad1e03b20a85

static page_t *kernelPageDirectory;

extern uint32_t KERNEL_PAGE_DIRECTORY;
extern uint32_t KERNEL_PAGE_TEMP;

static uint32_t paging_calculate_table(page_t virtAddr) {
    return virtAddr / PAGE_SIZE_4M;
}

static uint32_t paging_calculate_entry(page_t virtAddr) {
    return virtAddr % PAGE_SIZE_4M / PAGE_SIZE_4K;
}

void paging_change_directory(page_t directoryPhysicalAddr) {
    // Tell CPU the directory and enable paging.
    asm volatile ("mov %%eax, %%cr3": :"a"(directoryPhysicalAddr)); 
    asm volatile ("mov %cr0, %eax");
    asm volatile ("orl $0x80000000, %eax");
    asm volatile ("mov %eax, %cr0");
}

void paging_flush_tlb() {
    // Flush TLB.
    asm volatile ("mov %cr3, %eax");
    asm volatile ("mov %eax, %cr3");
}

void paging_flush_tlb_address(page_t address) {
    // Flush specified address in TLB.
    asm volatile ("invlpg (%0)" : : "b"(address) : "memory");
}

void paging_map_virtual_to_phys(page_t *directory, page_t virt, page_t phys) {
    // Calculate table and entry of virtual address.
    uint32_t tableIndex = paging_calculate_table(virt);
    uint32_t entryIndex = paging_calculate_entry(virt);

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.   
    if (MASK_PAGE_4K(directory[tableIndex]) == 0) {
        directory[tableIndex] = pmm_pop_page() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        paging_flush_tlb();
    }
    page_t *table = (page_t*)(PAGE_TABLE_ADDRESS_START + (tableIndex * PAGE_SIZE_4K));

    // Add address to table.
    table[entryIndex] = phys | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    paging_flush_tlb_address(virt);
}

void paging_map_kernel_virtual_to_phys(page_t virt, page_t phys) {
    paging_map_virtual_to_phys(kernelPageDirectory, virt, phys);
}

void paging_unmap_virtual(page_t *directory, page_t virt) {
    // Calculate table and entry of virtual address.
    uint32_t tableIndex = paging_calculate_table(virt);
    uint32_t entryIndex = paging_calculate_entry(virt);

    // Get pointer to table.
    page_t *table = (page_t*)(PAGE_TABLE_ADDRESS_START + (tableIndex * PAGE_SIZE_4K));

    // Clear address from table.
    table[entryIndex] = 0;
    paging_flush_tlb_address(virt);
}

void paging_unmap_kernel_virtual(page_t virt) {
    paging_unmap_virtual(kernelPageDirectory, virt);
}

static void paging_pagefault_handler() {
    

    page_t addr;
    asm volatile ("mov %%cr2, %0" : "=r"(addr));

    panic("Page fault at 0x%X!\n", addr);
}

void paging_init() {
    // Store paging addresses.
    memInfo.kernelPageDirectory = KERNEL_PAGE_DIRECTORY;
    memInfo.KernelPageTemp = KERNEL_PAGE_TEMP;

    // Get pointer to kernel page directory and create a new one.
    kernelPageDirectory = (page_t*)memInfo.kernelPageDirectory;
    for (uint32_t i = 0; i < 1024; i++)
        kernelPageDirectory[i] = 0;

    // Set last entry of directory to point to itself.
    kernelPageDirectory[PAGE_DIRECTORY_SIZE - 1] =
        (memInfo.kernelPageDirectory - memInfo.kernelVirtualOffset) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map kernel.
    for (uint32_t i = memInfo.kernelVirtualOffset; i <= memInfo.pageStackEnd; i+= PAGE_SIZE_4K) {
        paging_map_kernel_virtual_to_phys(i, i - memInfo.kernelVirtualOffset);
    }

    // Wire up handler for page faults and change to use our new page directory.
    interrupts_isr_install_handler(ISR_EXCEPTION_PAGE_FAULT, paging_pagefault_handler);
    paging_change_directory(((uint32_t)kernelPageDirectory) - memInfo.kernelVirtualOffset);

    // Zero out temporary table.
    memset((uint32_t*)memInfo.KernelPageTemp, 0, PAGE_SIZE_4K);

    // Pop physical page for test.
    page_t page = pmm_pop_page();
    kprintf("Popped page 0x%X for test...\n", page);
    
    // Map physical page to 0x1000 for testing.
    paging_map_kernel_virtual_to_phys(0x1000, page);

    // Test memory at location.
    kprintf("Testing memory at virtual address 0x1000...\n");
    page_t *testPage = (page_t*)0x1000;
    for (page_t i = 0; i < PAGE_TABLE_SIZE; i++)
        testPage[i] = i;

    bool pass = true;
    for (page_t i = 0; i < PAGE_TABLE_SIZE; i++)
        if (testPage[i] != i) {
            pass = false;
            break;
        }
    kprintf("Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("Memory test of virtual address 0x1000 failed.\n");

    // Unmap virtual address and return page to stack.
    kprintf("Unmapping 0x1000 and pushing page 0x%X back to stack...\n", page);
    paging_unmap_kernel_virtual(0x1000);
    pmm_push_page(page);

    kprintf("Paging initialized!\n");
}
