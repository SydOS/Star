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

void paging_map_kernel_virtual_to_phys(page_t virt, page_t phys) {
    paging_map_virtual_to_phys(kernelPageDirectory, virt, phys);
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

static void paging_pagefault_handler(registers_t *regs) {
    

    page_t addr;
    asm volatile ("mov %%cr2, %0" : "=r"(addr));

    panic("Page fault at 0x%X!\n", addr);
}

void paging_init() {
    // Store paging addresses.
    memInfo.kernelPageDirectory = KERNEL_PAGE_DIRECTORY;
    memInfo.KernelPageTemp = KERNEL_PAGE_TEMP;

    // Get pointer to kernel page directory made earlier in boot.
    kernelPageDirectory = (page_t*)memInfo.kernelPageDirectory;

    // Set last entry of directory to point to itself.
    kernelPageDirectory[PAGE_DIRECTORY_SIZE - 1] =
        (memInfo.kernelPageDirectory - memInfo.kernelVirtualOffset) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map temporary page table at virtual address 0x0, replacing the identity-mapped first 4MB.
    kernelPageDirectory[0] = memInfo.KernelPageTemp - memInfo.kernelVirtualOffset | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    page_t *tempPageTable = (page_t*)(memInfo.KernelPageTemp);
    paging_flush_tlb();

    // Pop a page and map it to 0x00000000.
    page_t pageTemp = pmm_pop_page();
    tempPageTable[0] = pageTemp | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Flush TLB to ensure changes take effect.
    paging_flush_tlb();
    page_t *kernelPageTable = (page_t*)0;

    // Re-map low memory and kernel using pages.
    uint32_t offset = 0;
    uint32_t currentPage = 0;
    for (uint32_t i = 0; i <= ((memInfo.pageStackEnd - memInfo.kernelVirtualOffset) / PAGE_SIZE_4K); i++, currentPage += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (i > 0 && i % 1024 == 0) {
            kernelPageDirectory[paging_calculate_table(memInfo.kernelVirtualOffset) + offset] = pageTemp | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

            // Pop another page and map it.
            pageTemp = pmm_pop_page();
            tempPageTable[0] = pageTemp | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
            paging_flush_tlb();

            // Increase offset and add new table to directory.
            offset++;      
        }
        kernelPageTable[i-(offset*1024)] = currentPage | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Zero out temporary table.
    kernelPageDirectory[0] = 0;
    tempPageTable[0] = 0;
    paging_flush_tlb();

    // Wire up handler for page faults.
    interrupts_isr_install_handler(ISR_EXCEPTION_PAGE_FAULT, paging_pagefault_handler);

    // Enable paging.
    paging_change_directory(((uint32_t)kernelPageDirectory) - memInfo.kernelVirtualOffset);

    // Test. TODO: unmap this after done.
    paging_map_virtual_to_phys(kernelPageDirectory, 0x1000, pmm_pop_page());
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

    kprintf("Paging initialized!\n");
}
