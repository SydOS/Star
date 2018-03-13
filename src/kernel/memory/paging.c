#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <io.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <arch/i386/kernel/cpuid.h>
#include <arch/i386/kernel/interrupts.h>

// http://www.rohitab.com/discuss/topic/31139-tutorial-paging-memory-mapping-with-a-recursive-page-directory/
// https://forum.osdev.org/viewtopic.php?f=15&t=19387
// https://medium.com/@connorstack/recursive-page-tables-ad1e03b20a85

extern bool PAE_ENABLED;

static uint32_t paging_calculate_table(page_t virtAddr) {
    return virtAddr / PAGE_SIZE_4M;
}

static uint32_t paging_calculate_entry(page_t virtAddr) {
    return virtAddr % PAGE_SIZE_4M / PAGE_SIZE_4K;
}

static uint32_t paging_pae_calculate_directory(page_t virtAddr) {
    return virtAddr / PAGE_SIZE_1G;
}

static uint32_t paging_pae_calculate_table(page_t virtAddr) {
    return (virtAddr - (paging_pae_calculate_directory(virtAddr) * PAGE_SIZE_1G)) / PAGE_SIZE_2M;
}

static uint32_t paging_pae_calculate_entry(page_t virtAddr) {
    return virtAddr % PAGE_SIZE_2M / PAGE_SIZE_4K;
}

void paging_change_directory(uintptr_t directoryPhysicalAddr) {
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

void paging_flush_tlb_address(uintptr_t address) {
    // Flush specified address in TLB.
    asm volatile ("invlpg (%0)" : : "b"(address) : "memory");
}

static uint64_t paging_get_pae_directory_address(uint32_t directoryIndex) {
    switch (directoryIndex) {
        case 0:
            return (uint64_t)PAGE_PAE_DIR_0GB_ADDRESS;
        case 1:
            return (uint64_t)PAGE_PAE_DIR_1GB_ADDRESS;
        case 2:
            return (uint64_t)PAGE_PAE_DIR_2GB_ADDRESS;
        case 3:
            return (uint64_t)PAGE_PAE_DIR_3GB_ADDRESS;
        default:
            panic("Invalid PAE page directory index specified.\n");
    }
}

static uint64_t paging_get_pae_tables_address(uint32_t directoryIndex) {
    switch (directoryIndex) {
        case 0:
            return (uint64_t)PAGE_PAE_TABLES_0GB_ADDRESS;
        case 1:
            return (uint64_t)PAGE_PAE_TABLES_1GB_ADDRESS;
        case 2:
            return (uint64_t)PAGE_PAE_TABLES_2GB_ADDRESS;
        case 3:
            return (uint64_t)PAGE_PAE_TABLES_3GB_ADDRESS;
        default:
            panic("Invalid PAE page directory index specified.\n");
    }
}

static void paging_map_std(page_t virtual, page_t physical) {
    // Get pointer to page directory.
    uint32_t *directory = (uint32_t*)( PAGE_DIR_ADDRESS );

    // Calculate table and entry of virtual address.
    uint32_t tableIndex = paging_calculate_table(virtual);
    uint32_t entryIndex = paging_calculate_entry(virtual);

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    if (MASK_PAGE_4K(directory[tableIndex]) == 0) {
        directory[tableIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        paging_flush_tlb();
    }
    uint32_t *table = (uint32_t*)(PAGE_TABLES_ADDRESS + (tableIndex * PAGE_SIZE_4K));

    // Add address to table.
    table[entryIndex] = physical == 0 ? 0 : (physical | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT);
}

static void paging_map_pae(page_t virtual, page_t physical) {
    // Get pointer to PDPT.
    uint64_t *directoryPointerTable = (uint64_t*)(PAGE_PAE_PDPT_ADDRESS);

    // Calculate directory, table, entry of virtual address.
    uint32_t dirIndex   = paging_pae_calculate_directory(virtual);
    uint32_t tableIndex = paging_pae_calculate_table(virtual);
    uint32_t entryIndex = paging_pae_calculate_entry(virtual);

    // Get address of directory from PDPT.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directory = (uint64_t*)paging_get_pae_directory_address(dirIndex);
    if (MASK_DIRECTORY_PAE(directoryPointerTable[dirIndex]) == 0) {
        // Pop page for new directory.
        uint64_t directoryFrameAddr = pmm_pop_frame();
        directoryPointerTable[dirIndex] = directoryFrameAddr | PAGING_PAGE_PRESENT;
        paging_flush_tlb();

        // Get pointer to 2GB directory, and map in the new directory.
        uint64_t *kernelDirectory = (uint64_t*)PAGE_PAE_DIR_2GB_ADDRESS;
        kernelDirectory[PAGE_PAE_DIRECTORY_SIZE - 3 + dirIndex] = directoryFrameAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        paging_flush_tlb_address(paging_get_pae_directory_address(dirIndex));

        // Zero out new directory.
        memset(directory, 0, PAGE_SIZE_4K);
    }

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint64_t *table = (uint64_t*)(paging_get_pae_tables_address(dirIndex) + (tableIndex * PAGE_SIZE_4K)); 
    if (MASK_PAGE_PAE_4K(directory[tableIndex]) == 0) {
        // Pop page frame for new table.
        directory[tableIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        paging_flush_tlb();

        // Zero out new table.
        memset(table, 0, PAGE_SIZE_4K);
    }
    
    // Add address to table.
    table[entryIndex] = physical == 0 ? 0 : (MASK_PAGE_PAE_4K(physical) | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT);
}

void paging_map_virtual_to_phys(page_t virtual, page_t physical) {
    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        paging_map_pae(virtual, physical);
    else
        paging_map_std(virtual, physical);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

void paging_unmap_virtual(page_t virtual) {
    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        paging_map_pae(virtual, 0);
    else
        paging_map_std(virtual, 0);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

bool paging_get_phys_std(page_t virtual, uint32_t *physOut) {
    // Get pointer to page directory.
    uint32_t *directory = (uint32_t*)(PAGE_DIR_ADDRESS);

    // Calculate table and entry of virtual address.
    uint32_t tableIndex = paging_calculate_table(virtual);
    uint32_t entryIndex = paging_calculate_entry(virtual);

    // Get address of table from directory.
    // If there isn't one, no virtual to physical mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    if (MASK_PAGE_4K(directory[tableIndex]) == 0) {
        return false;
    }
    uint32_t *table = (uint32_t*)(PAGE_TABLES_ADDRESS + (tableIndex * PAGE_SIZE_4K));

    // Is page present?
    if (!(table[entryIndex] & PAGING_PAGE_PRESENT))
        return false;

    // Get address from table.
    *physOut = MASK_PAGE_4K(table[entryIndex]);
    return true;
}

bool paging_get_phys_pae(page_t virtual, uint64_t *physOut) {
    // Get pointer to PDPT.
    uint64_t *directoryPointerTable = (uint64_t*)(PAGE_PAE_PDPT_ADDRESS);

    // Calculate directory, table, entry of virtual address.
    uint32_t dirIndex   = paging_pae_calculate_directory(virtual);
    uint32_t tableIndex = paging_pae_calculate_table(virtual);
    uint32_t entryIndex = paging_pae_calculate_entry(virtual);

    // Get address of directory from PDPT.
    // If there isn't one, no mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directory = (uint64_t*)paging_get_pae_directory_address(dirIndex);
    if (MASK_DIRECTORY_PAE(directoryPointerTable[dirIndex]) == 0)
        return false;

    // Get address of table from directory.
    // If there isn't one, no mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint64_t *table = (uint64_t*)(paging_get_pae_tables_address(dirIndex) + (tableIndex * PAGE_SIZE_4K)); 
    if (MASK_PAGE_PAE_4K(directory[tableIndex]) == 0)
        return false;

    // Is page present?
    if (!(table[entryIndex] & PAGING_PAGE_PRESENT))
        return false;
    
    // Get address from table.
    *physOut = MASK_PAGE_PAE_4K(table[entryIndex]);
    return true;
}

bool paging_get_phys(page_t virtual, uint64_t *physOut) {
    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        return paging_get_phys_pae(virtual, physOut);
    else
        return paging_get_phys_std(virtual, (uint32_t*)physOut);
}

void paging_map_region(page_t *directory, page_t startAddress, page_t endAddress, bool kernel, bool writeable) {
    // Ensure addresses are on 4KB boundaries.
    startAddress = MASK_PAGE_4K(startAddress);
    endAddress = MASK_PAGE_4K(endAddress);

    // Map space.
    
}

static void paging_pagefault_handler(registers_t *regs) {
    page_t addr;
    asm volatile ("mov %%cr2, %0" : "=r"(addr));
    kprintf("EAX: 0x%X, EBX: 0x%X, ECX: 0x%X, EDX: 0x%X\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("ESI: 0x%X, EDI: 0x%X, EBP: 0x%X, ESP: 0x%X\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    kprintf("EIP: 0x%X, EFLAGS: 0x%X\n", regs->eip, regs->eflags);
    panic("Page fault at 0x%X!\n", addr);
}

static void paging_late() {
    kprintf("Initializing standard 32-bit paging!\n");

    // Get pointer to the early-paging page table for 0x0.
    uint32_t *earlyPageTableLow = (uint32_t*)(PAGE_TABLES_ADDRESS);

    // Pop a new page frame for the page directory, and map it to 0x0 in the current virtual space.
    memInfo.kernelPageDirectory = pmm_pop_frame();
    earlyPageTableLow[0] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Get pointer to the page directory.
    uint32_t *pageDirectory = (uint32_t*)0x0;
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create the first page table for the kernel, and map it to 0x1000 in the current virtual space.
    uint32_t pageKernelTableAddr = pmm_pop_frame();
    earlyPageTableLow[1] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Get pointer to page table.
    uint32_t *pageKernelTable = (uint32_t*)0x1000;
    memset(pageKernelTable, 0, PAGE_SIZE_4K);

    // Add the table to the new directory.
    pageDirectory[paging_calculate_table(memInfo.kernelVirtualOffset)] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint32_t page = 0; page <= memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_4M == 0) { 
            // Create another table and map to 0x1000 in the current virtual space.
            pageKernelTableAddr = pmm_pop_frame();
            earlyPageTableLow[1] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
            paging_flush_tlb();

            // Zero out new page table.
            memset(pageKernelTable, 0, PAGE_SIZE_4K);

            // Increase offset and add the table to our new directory.
            offset++;
            pageDirectory[paging_calculate_table(memInfo.kernelVirtualOffset) + offset] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }

        // Add page to table.
        pageKernelTable[(page / PAGE_SIZE_4K) - (offset * PAGE_DIRECTORY_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Recursively map page directory to last entry.
    pageDirectory[PAGE_DIRECTORY_SIZE - 1] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
}

static void paging_pae_late() {
    kprintf("Initializing PAE paging!\n");

    // Get pointer to the early-paging page table for 0x0.
    uint64_t *earlyPageTableLow = (uint64_t*)(PAGE_SIZE_1G * 3 + (PAGE_SIZE_2M * (PAGE_PAE_DIRECTORY_SIZE - 4)));

    // Pop a new page frame for the PDPT, and map it to 0x0 in the current virtual space.
    memInfo.kernelPageDirectory = pmm_pop_frame();
    earlyPageTableLow[0] = (uint64_t)memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Get pointer to the PDPT.
    uint64_t *pageDirectoryPointerTable = (uint64_t*)0x0;
    memset(pageDirectoryPointerTable, 0, PAGE_SIZE_4K);

    // Pop a new page for the 3GB page directory, which will hold the kernel at 0xC0000000.
    // This is mapped to 0x1000 in the current virtual address space.
    uint64_t pageDirectoryAddr = pmm_pop_frame();
    earlyPageTableLow[1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    pageDirectoryPointerTable[3] = pageDirectoryAddr | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Get pointer to the page directory.
    uint64_t *pageDirectory = (uint64_t*)0x1000;
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create the first page table for the kernel, and map it to 0x2000 in the current virtual space.
    uint64_t pageKernelTableAddr = pmm_pop_frame();
    earlyPageTableLow[2] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Get pointer to page table.
    uint64_t *pageKernelTable = (uint64_t*)0x2000;
    memset(pageKernelTable, 0, PAGE_SIZE_4K);

    // Add the table to the new directory.
    pageDirectory[0] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint64_t page = 0; page <= memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_2M == 0) { 
            // Create another table and map to 0x2000 in the current virtual space.
            pageKernelTableAddr = pmm_pop_frame();
            earlyPageTableLow[2] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
            paging_flush_tlb();

            // Zero out new page table.
            memset(pageKernelTable, 0, PAGE_SIZE_4K);

            // Increase offset and add the table to our new directory.
            offset++;
            pageDirectory[offset] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        }

        // Add page to table.
        pageKernelTable[(page / PAGE_SIZE_4K) - (offset * PAGE_PAE_DIRECTORY_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Recursively map kernel page directory.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT; // 3GB directory.

    // Create 2GB page directory.
    pageDirectoryAddr = pmm_pop_frame();
    earlyPageTableLow[1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    pageDirectoryPointerTable[2] = pageDirectoryAddr | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Zero out new directory.
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Map the 2GB page directory and the PDPT recursively.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = (uint64_t)pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT; // 2GB directory.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 4] = (uint64_t)memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT; // PDPT.

    // Detect and enable the NX bit if supported.
    uint32_t result, unused;
    if (cpuid_query(CPUID_INTELFEATURES, &unused, &unused, &unused, &result) && (result & CPUID_FEAT_EDX_NX)) {
        // Enable NX bit.
        uint64_t msr = cpu_msr_read(0xC0000080);
        cpu_msr_write(0xC0000080, msr | 0x800);
        memInfo.nxEnabled = true;
        kprintf("NX bit enabled!\n");
    }
}

void paging_init() {
    // Wire up page fault handler.
    interrupts_isr_install_handler(ISR_EXCEPTION_PAGE_FAULT, paging_pagefault_handler);

    // Is PAE enabled?
    memInfo.paeEnabled = PAE_ENABLED;
    if (memInfo.paeEnabled)
        paging_pae_late(); // Use PAE paging.
    else 
        paging_late(); // No PAE, using standard paging.
        
    // Change to use our new page directory.
    paging_change_directory(memInfo.kernelPageDirectory);

    // Pop physical page for test.
    page_t page = pmm_pop_frame();
    kprintf("Popped page 0x%X for test...\n", page);
    
    // Map physical page to 0x1000 for testing.
    paging_map_virtual_to_phys(0x1000, page);

    // Test memory at location.
    kprintf("Testing memory at virtual address 0x1000...\n");
    uintptr_t *testPage = (uintptr_t*)0x1000;
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++)
        testPage[i] = i;

    bool pass = true;
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++)
        if (testPage[i] != i) {
            pass = false;
            break;
        }
    kprintf("Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("Memory test of virtual address 0x1000 failed.\n");

    // Unmap virtual address and return page to stack.
    kprintf("Unmapping 0x1000 and pushing page 0x%X back to stack...\n", page);
    paging_unmap_virtual(0x1000);
    pmm_push_frame(page);

    kprintf("Paging initialized!\n");
}
