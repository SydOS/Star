/*
 * File: paging_i386.c
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

static uint32_t paging_calculate_table(uintptr_t virtAddr) {
    return virtAddr / PAGE_SIZE_4M;
}

static uint32_t paging_calculate_entry(uintptr_t virtAddr) {
    return virtAddr % PAGE_SIZE_4M / PAGE_SIZE_4K;
}

static uint32_t paging_pae_calculate_directory(uintptr_t virtAddr) {
    return virtAddr / PAGE_SIZE_1G;
}

static uint32_t paging_pae_calculate_table(uintptr_t virtAddr) {
    return (virtAddr - (paging_pae_calculate_directory(virtAddr) * PAGE_SIZE_1G)) / PAGE_SIZE_2M;
}

static uint32_t paging_pae_calculate_entry(uintptr_t virtAddr) {
    return virtAddr % PAGE_SIZE_2M / PAGE_SIZE_4K;
}

static uint32_t paging_get_pae_directory_address(uint32_t directoryIndex) {
    switch (directoryIndex) {
        case 0:
            return (uint32_t)PAGE_PAE_DIR_0GB_ADDRESS;
        case 1:
            return (uint32_t)PAGE_PAE_DIR_1GB_ADDRESS;
        case 2:
            return (uint32_t)PAGE_PAE_DIR_2GB_ADDRESS;
        case 3:
            return (uint32_t)PAGE_PAE_DIR_3GB_ADDRESS;
        default:
            panic("Invalid PAE page directory index specified.\n");
            return 0;
    }
}

static uint32_t paging_get_pae_tables_address(uint32_t directoryIndex) {
    switch (directoryIndex) {
        case 0:
            return (uint32_t)PAGE_PAE_TABLES_0GB_ADDRESS;
        case 1:
            return (uint32_t)PAGE_PAE_TABLES_1GB_ADDRESS;
        case 2:
            return (uint32_t)PAGE_PAE_TABLES_2GB_ADDRESS;
        case 3:
            return (uint32_t)PAGE_PAE_TABLES_3GB_ADDRESS;
        default:
            panic("Invalid PAE page directory index specified.\n");
            return 0;
    }
}

static void paging_map_std(uintptr_t virtual, uint32_t physical, bool unmap) {
    // Get pointer to page directory.
    uint32_t *directory = (uint32_t*)( PAGE_DIR_ADDRESS );

    // Calculate table and entry of virtual address.
    uint32_t tableIndex = paging_calculate_table(virtual);
    uint32_t entryIndex = paging_calculate_entry(virtual);

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint32_t *table = (uint32_t*)(PAGE_TABLES_ADDRESS + (tableIndex * PAGE_SIZE_4K));
    if (MASK_PAGE_4K(directory[tableIndex]) == 0) {
        directory[tableIndex] = pmm_pop_frame() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb();

        // Zero out new table.
        memset(table, 0, PAGE_SIZE_4K);
    }

    // Add address to table.
    table[entryIndex] = unmap ? 0 : physical;
}

static void paging_map_pae(uintptr_t virtual, uint64_t physical, bool unmap) {
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
        uint64_t directoryFrameAddr = pmm_pop_frame_nonpae();
        directoryPointerTable[dirIndex] = directoryFrameAddr | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb();

        // Get pointer to 2GB directory, and map in the new directory.
        uint64_t *kernelDirectory = (uint64_t*)PAGE_PAE_DIR_2GB_ADDRESS;
        kernelDirectory[PAGE_PAE_DIRECTORY_SIZE - 3 + dirIndex] = directoryFrameAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        paging_flush_tlb_address(paging_get_pae_directory_address(dirIndex));

        // Zero out new directory.
        memset(directory, 0, PAGE_SIZE_4K);
    }

    // Get address of table from directory.
    // If there isn't one, create one.
    // Pages will never be located at 0x0, so its safe to assume a value of 0 = no table defined.
    uint64_t *table = (uint64_t*)(paging_get_pae_tables_address(dirIndex) + (tableIndex * PAGE_SIZE_4K)); 
    if (MASK_PAGE_4K_64BIT(directory[tableIndex]) == 0) {
        // Pop page frame for new table.
        directory[tableIndex] = pmm_pop_frame_nonpae() | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
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

    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        paging_map_pae(virtual, physical | flags, false);
    else
        paging_map_std(virtual, physical | flags, false);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

void paging_unmap(uintptr_t virtual) {
    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        paging_map_pae(virtual, 0, true);
    else
        paging_map_std(virtual, 0, true);

    // Flush TLB
    paging_flush_tlb_address(virtual);
}

static bool paging_get_phys_std(uintptr_t virtual, uint64_t *physOut) {
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

static bool paging_get_phys_pae(uintptr_t virtual, uint64_t *physOut) {
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
    if (MASK_PAGE_4K_64BIT(directory[tableIndex]) == 0)
        return false;

    // Is page present?
    if (!(table[entryIndex] & PAGING_PAGE_PRESENT))
        return false;
    
    // Get address from table.
    *physOut = MASK_PAGE_4K_64BIT(table[entryIndex]);
    return true;
}

bool paging_get_phys(uintptr_t virtual, uint64_t *physOut) {
    // Are we in PAE mode?
    if (memInfo.paeEnabled)
        return paging_get_phys_pae(virtual, physOut);
    else
        return paging_get_phys_std(virtual, physOut);
}

uintptr_t paging_create_app_copy(void) {
    uint64_t appDirPage = pmm_pop_frame_nonpae();

    // Are we in PAE mode?
    if (memInfo.paeEnabled) {
        // PAE mode.
        // Create a new PDPT.
        uint64_t *appPointerTable = (uint64_t*)paging_device_alloc(appDirPage, appDirPage);
        memset(appPointerTable, 0, PAGE_SIZE_4K);

        // Get pointer to current PDPT.
        uint64_t *directoryPointerTable = (uint64_t*)(PAGE_PAE_PDPT_ADDRESS);

        // Copy higher half of PDPT into the new one. We can safely assume the current one's higher-half is accurate.
        uint32_t dirIndex = paging_pae_calculate_directory(memInfo.kernelVirtualOffset);
        appPointerTable[dirIndex] = directoryPointerTable[dirIndex];

        // Create 2GB page directory.
        uint32_t pageDirectoryAddr = pmm_pop_frame_nonpae();
        appPointerTable[2] = pageDirectoryAddr | PAGING_PAGE_PRESENT;
        paging_flush_tlb();

        // Zero out new directory.
        uint64_t *appLowPageDirectory = (uint64_t*)paging_device_alloc(pageDirectoryAddr, pageDirectoryAddr);
        memset(appLowPageDirectory, 0, PAGE_SIZE_4K);

        // Map the 2GB page directory and the PDPT recursively.
        appLowPageDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = (uint64_t)pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER; // 2GB directory.
        appLowPageDirectory[PAGE_PAE_DIRECTORY_SIZE - 4] = (uint64_t)appDirPage | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER; // PDPT.

        // Unmap.
        paging_device_free((uintptr_t)appLowPageDirectory, (uintptr_t)appLowPageDirectory);
        paging_device_free((uintptr_t)appPointerTable, (uintptr_t)appPointerTable);
    }
    else {
        // Standard mode.
        // Create a new paging directory.
        uint32_t *appPageDir = (uint32_t*)paging_device_alloc(appDirPage, appDirPage);
        memset(appPageDir, 0, PAGE_SIZE_4K);

        // Get pointer to current page directory.
        uint32_t *directory = (uint32_t*)(PAGE_DIR_ADDRESS);

        // Copy higher half of page directory into the new one. We can safely assume the current one's higher-half is accurate.
        for (uint16_t i = paging_calculate_table(memInfo.kernelVirtualOffset); i < PAGE_DIRECTORY_SIZE; i++)
            appPageDir[i] = directory[i];

        // Recursively map page directory to last entry.
        appPageDir[PAGE_DIRECTORY_SIZE - 1] = (uint32_t)appDirPage | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

        // Unmap directory.
        paging_device_free((uintptr_t)appPageDir, (uintptr_t)appPageDir);
    }

    return (uintptr_t)appDirPage;
}

void paging_late_std() {
    kprintf("PAGING: Initializing standard 32-bit paging!\n");

    // Get pointer to the early-paging page table for 0x0.
    uint32_t *earlyPageTableLow = (uint32_t*)(PAGE_TABLES_ADDRESS);

    // Pop a new page frame for the page directory, and map it to 0x0 in the current virtual space.
    memInfo.kernelPageDirectory = pmm_pop_frame();
    earlyPageTableLow[0] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

    // Get pointer to the page directory.
    uint32_t *pageDirectory = (uint32_t*)0x0;
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create the first page table for the kernel, and map it to 0x1000 in the current virtual space.
    uint32_t pageKernelTableAddr = pmm_pop_frame();
    earlyPageTableLow[1] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Get pointer to page table.
    uint32_t *pageKernelTable = (uint32_t*)0x1000;
    memset(pageKernelTable, 0, PAGE_SIZE_4K);

    // Add the table to the new directory.
    pageDirectory[paging_calculate_table(memInfo.kernelVirtualOffset)] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint32_t page = 0; page <= memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_4M == 0) { 
            // Create another table and map to 0x1000 in the current virtual space.
            pageKernelTableAddr = pmm_pop_frame();
            earlyPageTableLow[1] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
            paging_flush_tlb();

            // Zero out new page table.
            memset(pageKernelTable, 0, PAGE_SIZE_4K);

            // Increase offset and add the table to our new directory.
            offset++;
            pageDirectory[paging_calculate_table(memInfo.kernelVirtualOffset) + offset] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
        }

        // Add page to table.
        pageKernelTable[(page / PAGE_SIZE_4K) - (offset * PAGE_DIRECTORY_SIZE)] = page | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    }

    // Fill up rest of directory with empty tables.
    for (offset = offset + paging_calculate_table(memInfo.kernelVirtualOffset) + 1; offset < PAGE_DIRECTORY_SIZE - 1; offset++) {
        // Create another table and map to 0x1000 in the current virtual space.
        pageKernelTableAddr = pmm_pop_frame();
        earlyPageTableLow[1] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
        paging_flush_tlb();

        // Zero out new page table.
        memset(pageKernelTable, 0, PAGE_SIZE_4K);
        pageDirectory[offset] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    }

    // Recursively map page directory to last entry.
    pageDirectory[PAGE_DIRECTORY_SIZE - 1] = memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
}

void paging_late_pae() {
    kprintf("PAGING: Initializing PAE paging!\n");

    // Get pointer to the early-paging page table for 0x0.
    uint64_t *earlyPageTableLow = (uint64_t*)((uint32_t)PAGE_SIZE_1G * 3 + ((uint32_t)PAGE_SIZE_2M * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 4)));

    // Pop a new page frame for the PDPT, and map it to 0x0 in the current virtual space.
    // This must be a 32-bit address, so pull from the non-PAE stack.
    memInfo.kernelPageDirectory = pmm_pop_frame_nonpae();
    earlyPageTableLow[0] = (uint64_t)memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;

    // Get pointer to the PDPT.
    uint64_t *pageDirectoryPointerTable = (uint64_t*)0x0;
    memset(pageDirectoryPointerTable, 0, PAGE_SIZE_4K);

    // Pop a new page for the 3GB page directory, which will hold the kernel at 0xC0000000.
    // This is mapped to 0x1000 in the current virtual address space.
    uint64_t pageDirectoryAddr = pmm_pop_frame_nonpae();
    earlyPageTableLow[1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    pageDirectoryPointerTable[3] = pageDirectoryAddr | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Get pointer to the page directory.
    uint64_t *pageDirectory = (uint64_t*)0x1000;
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Create the first page table for the kernel, and map it to 0x2000 in the current virtual space.
    uint64_t pageKernelTableAddr = pmm_pop_frame_nonpae();
    earlyPageTableLow[2] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT;
    paging_flush_tlb();

    // Get pointer to page table.
    uint64_t *pageKernelTable = (uint64_t*)0x2000;
    memset(pageKernelTable, 0, PAGE_SIZE_4K);

    // Add the table to the new directory.
    pageDirectory[0] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;

    // Map low memory and kernel to higher-half virtual space.
    uint32_t offset = 0;
    for (uint64_t page = 0; page <= memInfo.pageFrameStackEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K) {
        // Have we reached the need to create another table?
        if (page > 0 && page % PAGE_SIZE_2M == 0) { 
            // Create another table and map to 0x2000 in the current virtual space.
            pageKernelTableAddr = pmm_pop_frame_nonpae();
            earlyPageTableLow[2] = pageKernelTableAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
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

    // Recursively map kernel page directory.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER; // 3GB directory.

    // Create 2GB page directory.
    pageDirectoryAddr = pmm_pop_frame_nonpae();
    earlyPageTableLow[1] = pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    pageDirectoryPointerTable[2] = pageDirectoryAddr | PAGING_PAGE_PRESENT | PAGING_PAGE_USER;
    paging_flush_tlb();

    // Zero out new directory.
    memset(pageDirectory, 0, PAGE_SIZE_4K);

    // Map the 2GB page directory and the PDPT recursively.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 1] = (uint64_t)pageDirectoryAddr | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER; // 2GB directory.
    pageDirectory[PAGE_PAE_DIRECTORY_SIZE - 4] = (uint64_t)memInfo.kernelPageDirectory | PAGING_PAGE_READWRITE | PAGING_PAGE_PRESENT | PAGING_PAGE_USER; // PDPT.

    // Detect and enable the NX bit if supported.
    uint32_t result, unused;
    if (cpuid_query(CPUID_INTELFEATURES, &unused, &unused, &unused, &result) && (result & CPUID_FEAT_EDX_NX)) {
        // Enable NX bit.
        uint64_t msr = cpu_msr_read(0xC0000080);
        cpu_msr_write(0xC0000080, msr | 0x800);
        memInfo.nxEnabled = true;
        kprintf("PAGING: NX bit enabled!\n");
    }
}
