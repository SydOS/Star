/*
 * File: paging_x86_64.c
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

#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <io.h>
#include <kernel/memory/paging.h>
#include <kernel/cpuid.h>

/**
 * Calculates the PDPT index.
 * @param virtAddr The address to use.
 * @return The PDPT index.
 */
static uint32_t paging_long_calculate_pdpt(uintptr_t virtAddr) {
    return virtAddr / PAGE_SIZE_512G;
}

/**
 * Calculates the directory index.
 * @param virtAddr The address to use.
 * @return The directory index.
 */
static uint32_t paging_long_calculate_directory(uintptr_t virtAddr) {
    return (virtAddr - (paging_long_calculate_pdpt(virtAddr) * PAGE_SIZE_512G)) / PAGE_SIZE_1G;
}

/**
 * Calculates the table index.
 * @param virtAddr The address to use.
 * @return The table index.
 */
static uint32_t paging_long_calculate_table(uintptr_t virtAddr) {
    return (virtAddr - (paging_long_calculate_pdpt(virtAddr) * PAGE_SIZE_512G) - (paging_long_calculate_directory(virtAddr) * PAGE_SIZE_1G)) / PAGE_SIZE_2M;
}

/**
 * Calculates the entry index.
 * @param virtAddr The address to use.
 * @return The entry index.
 */
static uint32_t paging_long_calculate_entry(uintptr_t virtAddr) {
    return virtAddr % PAGE_SIZE_2M / PAGE_SIZE_4K;
}

static void paging_map_long(uintptr_t virtual, uint64_t physical, bool unmap) {
    // If the address is canonical, strip off the leading 0xFFFF.
    if (virtual & 0xFFFF000000000000)
        virtual &= 0x0000FFFFFFFFFFFF;

    // Get pointer to PML4.
    uint64_t *pml4Table = (uint64_t*)PAGE_LONG_PML4_ADDRESS;

    // Calculate PDPT, directory, table, entry of virtual address.
    uint32_t pdptIndex  = paging_long_calculate_pdpt(virtual);
    uint32_t dirIndex   = paging_long_calculate_directory(virtual);
    uint32_t tableIndex = paging_long_calculate_table(virtual);
    uint32_t entryIndex = paging_long_calculate_entry(virtual);

    // Get address of PDPT from PML4 table.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directoryPointerTable = (uint64_t*)PAGE_LONG_PDPT_ADDRESS(pdptIndex);
    if (MASK_PAGE_4K(pml4Table[pdptIndex]) == 0) {
        // Pop page for new PDPT.
        pml4Table[pdptIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb();

        // Zero out new directory.
        memset(directoryPointerTable, 0, PAGE_SIZE_4K);
    }

    // Get address of directory from PDPT.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directory = (uint64_t*)PAGE_LONG_DIR_ADDRESS(pdptIndex, dirIndex);
    if (MASK_PAGE_4K(directoryPointerTable[dirIndex]) == 0) {
        // Pop page for new directory.
        directoryPointerTable[dirIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb();

        // Zero out new directory.
        memset(directory, 0, PAGE_SIZE_4K);
    }

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint64_t *table = (uint64_t*)(PAGE_LONG_TABLE_ADDRESS(pdptIndex, dirIndex, tableIndex)); 
    if (MASK_PAGE_4K(directory[tableIndex]) == 0) {
        // Pop page frame for new table.
        directory[tableIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb();

        // Zero out new table.
        memset(table, 0, PAGE_SIZE_4K);
    }
    
    // Add address to table.
    table[entryIndex] = unmap ? 0 : physical;
}

void paging_map(uintptr_t virtual, uint64_t physical, bool kernel, bool writeable) {
    // Determine flags.
    uint64_t flags = 0;
    if (!kernel)
        flags |= PAGING_PAGE_USER;
    if (writeable)
        flags |= PAGING_PAGE_READWRITE;
    flags |= PAGING_PAGE_PRESENT;

    // Map address.
    paging_map_long(virtual, physical | flags, false);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

void paging_unmap(uintptr_t virtual) {
    // Map address.
    paging_map_long(virtual, 0, true);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

bool paging_get_phys(uintptr_t virtual, uint64_t *physOut) {
    // If the address is canonical, strip off the leading 0xFFFF.
    if (virtual & 0xFFFF000000000000)
        virtual &= 0x0000FFFFFFFFFFFF;

    // Get pointer to PML4.
    uint64_t *pml4Table = (uint64_t*)PAGE_LONG_PML4_ADDRESS;

    // Calculate PDPT, directory, table, entry of virtual address.
    uint32_t pdptIndex  = paging_long_calculate_pdpt(virtual);
    uint32_t dirIndex   = paging_long_calculate_directory(virtual);
    uint32_t tableIndex = paging_long_calculate_table(virtual);
    uint32_t entryIndex = paging_long_calculate_entry(virtual);

    // Get address of PDPT from PML4 table.
    // If there isn't one, no mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directoryPointerTable = (uint64_t*)PAGE_LONG_PDPT_ADDRESS(pdptIndex);
    if (MASK_PAGE_4K(pml4Table[pdptIndex]) == 0)
        return false;

    // Get address of directory from PDPT.
    // If there isn't one, no mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no directory defined.
    uint64_t* directory = (uint64_t*)PAGE_LONG_DIR_ADDRESS(pdptIndex, dirIndex);
    if (MASK_PAGE_4K(directoryPointerTable[dirIndex]) == 0)
        return false;

    // Get address of table from directory.
    // If there isn't one, no mapping exists.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint64_t *table = (uint64_t*)(PAGE_LONG_TABLE_ADDRESS(pdptIndex, dirIndex, tableIndex)); 
    if (MASK_PAGE_4K(directory[tableIndex]) == 0)
        return false;
    
    // Is page present?
    if (!(table[entryIndex] & PAGING_PAGE_PRESENT))
        return false;

    // Get address from table.
    *physOut = MASK_PAGE_4K(table[entryIndex]);
    return true;
}

uintptr_t paging_create_app_copy(void) {
    // Create a new PML4 table.
    uint64_t appPml4Page = pmm_pop_frame();
    uint64_t *appPml4Table = (uint64_t*)paging_device_alloc(appPml4Page, appPml4Page);
    memset(appPml4Table, 0, PAGE_SIZE_4K);

    // Get current PML4 table.
    uint64_t *pml4Table = (uint64_t*)PAGE_LONG_PML4_ADDRESS;

    // Copy higher half of PML4 table into the new one. We can safely assume the current one's higher-half is accurate.
    for (uint16_t i = (PAGE_LONG_STRUCT_SIZE / 2); i < PAGE_LONG_STRUCT_SIZE; i++)
        appPml4Table[i] = pml4Table[i];

    // Recursively map PML4 table.
    appPml4Table[PAGE_LONG_STRUCT_SIZE - 1] = appPml4Page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Unmap table.
    paging_device_free((uintptr_t)appPml4Table, (uintptr_t)appPml4Table);
    return appPml4Page;
}

/**
 * Sets up 4-level paging.
 */
void paging_late_long() {
    kprintf("PAGING: Initializing 4 level paging!\n");

    // Get pointer to the early-paging page table for 0x0.
    uint64_t *earlyPageTableLow = (uint64_t*)(PAGE_LONG_TABLES_ADDRESS);

    // Pop a new page frame for the PML4 table, and map it to 0x0 in the current virtual space.
    memInfo.kernelPageDirectory = pmm_pop_frame();
    earlyPageTableLow[0] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Get pointer to the PML4 table (mapped at 0x0) and zero it.
    uint64_t *pagePml4Table = (uint64_t*)0x0;
    memset(pagePml4Table, 0, PAGE_SIZE_4K);

    // Pop a new page frame for the 128TB PDPT, and map it to 0x1000 in the current virtual space.
    uint64_t pageDirectoryPointerTableAddr = pmm_pop_frame();
    earlyPageTableLow[1] = pageDirectoryPointerTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    pagePml4Table[256] = pageDirectoryPointerTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Get pointer to the 0GB PDPT and zero it.
    uint64_t *pageDirectoryPointerTable = (uint64_t*)0x1000;
    memset(pageDirectoryPointerTable, 0, PAGE_SIZE_4K);

    // Pop a new page for the 3GB page directory, which will hold the kernel at 0xC0000000.
    // This is mapped to 0x2000 in the current virtual address space.
    uint64_t pageDirectoryAddr = pmm_pop_frame();
    earlyPageTableLow[2] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    pageDirectoryPointerTable[0] = pageDirectoryAddr | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Get pointer to the page directory.
    uint64_t *pageDirectory = (uint64_t*)0x2000;
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create the first page table for the kernel, and map it to 0x3000 in the current virtual space.
    uint64_t pageKernelTableAddr = pmm_pop_frame();
    earlyPageTableLow[3] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Get pointer to page table.
    uint64_t *pageKernelTable = (uint64_t*)0x3000;
    memset(pageKernelTable, 0, PAGE_SIZE_4K);

    // Add the table to the new directory.
    pageDirectory[0] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint64_t page = 0; page <= memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_2M == 0) { 
            // Create another table and map to 0x2000 in the current virtual space.
            pageKernelTableAddr = pmm_pop_frame();
            earlyPageTableLow[3] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
            paging_flush_tlb();

            // Zero out new page table.
            memset(pageKernelTable, 0, PAGE_SIZE_4K);

            // Increase offset and add the table to our new directory.
            offset++;
            pageDirectory[offset] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        }

        // Add page to table.
        pageKernelTable[(page / PAGE_SIZE_4K) - (offset * PAGE_PAE_DIRECTORY_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    }

    // Recursively map PML4 table.
    pagePml4Table[PAGE_LONG_STRUCT_SIZE - 1] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

    // Detect and enable the NX bit if supported.
    uint32_t result, unused;
    if (cpuid_query(CPUID_INTELFEATURES, &unused, &unused, &unused, &result) && (result & CPUID_FEAT_EDX_NX)) {
        // Enable NX bit.
        uint64_t msr = cpu_msr_read(0xC0000080);
        cpu_msr_write(0xC0000080, msr | 0x800);
        memInfo.nxEnabled = true;
        kprintf("PAGING: NX bit enabled!\n");
    }
    kprintf("PAGING: 4-level paging initialized! PML4 table located at 0x%p.\n", memInfo.kernelPageDirectory);
}
