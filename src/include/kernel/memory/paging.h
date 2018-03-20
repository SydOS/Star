#ifndef PAGING_H
#define PAGING_H

#include <main.h>
#include <kernel/memory/pmm.h>

// Refer to Chapter 4: Paging of volume 3 of the Intel SDM for more info.

// Sizes.
#define PAGE_SIZE_4K                    0x1000
#define PAGE_SIZE_64K                   0x10000
#define PAGE_SIZE_2M		            0x200000
#define PAGE_SIZE_4M		            0x400000
#define PAGE_SIZE_1G		            0x40000000
#define PAGE_SIZE_4G                    0x100000000
#define PAGE_SIZE_512G                  0x8000000000
#define PAGE_DIRECTORY_SIZE             1024
#define PAGE_TABLE_SIZE                 1024
#define PAGE_TABLES_ADDRESS             ((uint32_t)PAGE_SIZE_4M * (uint32_t)(PAGE_DIRECTORY_SIZE - 1))
#define PAGE_DIR_ADDRESS                ((uint32_t)PAGE_TABLES_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_DIRECTORY_SIZE - 1)))

#define PAGE_0GB_ADDRESS                ((uint32_t)PAGE_SIZE_1G * 0)
#define PAGE_1GB_ADDRESS                ((uint32_t)PAGE_SIZE_1G * 1)
#define PAGE_2GB_ADDRESS                ((uint32_t)PAGE_SIZE_1G * 2)
#define PAGE_3GB_ADDRESS                ((uint32_t)PAGE_SIZE_1G * 3)

// PAE sizes.
#define PAGE_PAE_PDPT_SIZE              4
#define PAGE_PAE_DIRECTORY_SIZE         512
#define PAGE_PAE_TABLE_SIZE             512

#define PAGE_PAE_TABLES_3GB_ADDRESS     (((uint32_t)PAGE_3GB_ADDRESS) + ((uint32_t)PAGE_SIZE_2M * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 1)))
#define PAGE_PAE_TABLES_2GB_ADDRESS     (((uint32_t)PAGE_2GB_ADDRESS) + ((uint32_t)PAGE_SIZE_2M * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 1)))
#define PAGE_PAE_TABLES_1GB_ADDRESS     (((uint32_t)PAGE_2GB_ADDRESS) + ((uint32_t)PAGE_SIZE_2M * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 2)))
#define PAGE_PAE_TABLES_0GB_ADDRESS     (((uint32_t)PAGE_2GB_ADDRESS) + ((uint32_t)PAGE_SIZE_2M * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 3)))

#define PAGE_PAE_DIR_3GB_ADDRESS        ((uint32_t)PAGE_PAE_TABLES_3GB_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 1)))
#define PAGE_PAE_DIR_2GB_ADDRESS        ((uint32_t)PAGE_PAE_TABLES_2GB_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 1)))
#define PAGE_PAE_DIR_1GB_ADDRESS        ((uint32_t)PAGE_PAE_TABLES_2GB_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 2)))
#define PAGE_PAE_DIR_0GB_ADDRESS        ((uint32_t)PAGE_PAE_TABLES_2GB_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 3)))
#define PAGE_PAE_PDPT_ADDRESS           ((uint32_t)PAGE_PAE_TABLES_2GB_ADDRESS + ((uint32_t)PAGE_SIZE_4K * ((uint32_t)PAGE_PAE_DIRECTORY_SIZE - 4)))

// Long mode stuff.
#define PAGE_LONG_STRUCT_SIZE                                           512
#define PAGE_LONG_TABLES_ADDRESS                                        (((uint64_t)PAGE_SIZE_512G * (uint64_t)(PAGE_LONG_STRUCT_SIZE - 1)) + 0xFFFF000000000000)
#define PAGE_LONG_TABLE_ADDRESS(pdptIndex, directoryIndex, tableIndex)  ((uint64_t)PAGE_LONG_TABLES_ADDRESS + ((uint64_t)PAGE_SIZE_1G * (uint64_t)pdptIndex) + ((uint64_t)PAGE_SIZE_2M * (uint64_t)directoryIndex) + ((uint64_t)PAGE_SIZE_4K) * (uint64_t)tableIndex)
#define PAGE_LONG_DIR_ADDRESS(pdptIndex, directoryIndex)                ((uint64_t)PAGE_LONG_TABLE_ADDRESS(511, pdptIndex, directoryIndex))
#define PAGE_LONG_PDPT_ADDRESS(pdptIndex)                               ((uint64_t)PAGE_LONG_TABLE_ADDRESS(511, 511, pdptIndex))
#define PAGE_LONG_PML4_ADDRESS                                          ((uint64_t)PAGE_LONG_PDPT_ADDRESS(511))

// Masks.
#define MASK_DIRECTORY_PAE(addr)        ((uint64_t)(addr) & 0xFFFFFFF0)     // Get only the PDPT address.
#define MASK_PAGE_4K_64BIT(size)        ((uint64_t)(size) & 0xFFFFF000)     // Get only the page address.
#define MASK_PAGEFLAGS_4K_64BIT(size)   ((uint64_t)(size) & ~0xFFFFF000)    // Get only the page flags.

// Alignments.
#define ALIGN_4K(size)          	(((uint32_t)(size) + (uint32_t)PAGE_SIZE_4K) & 0xFFFFF000)
#define ALIGN_4K_64BIT(size)        (((uint64_t)(size) + (uint64_t)PAGE_SIZE_4K) & 0xFFFFFFFFFFFFF000)
#define ALIGN_64K(size)             (((uint32_t)(size) + (uint32_t)PAGE_SIZE_64K) & 0xFFFF0000)

// Mask macros.
#ifdef X86_64
#define MASK_PAGE_4K(size)          ((uint64_t)(size) & 0xFFFFFFFFFFFFF000)     // Get only the page address.
#define MASK_PAGEFLAGS_4K(size)     ((uint64_t)(size) & ~0xFFFFFFFFFFFFF000)    // Get only the page flags.
#else
#define MASK_PAGE_4K(size)          ((uint32_t)(size) & 0xFFFFF000)     // Get only the page address.
#define MASK_PAGEFLAGS_4K(size)     ((uint32_t)(size) & ~0xFFFFF000)    // Get only the page flags.
#endif

enum {
    PAGING_PAGE_PRESENT         = 0x01,
    PAGING_PAGE_READWRITE       = 0x02,
    PAGING_PAGE_USER            = 0x04,
    PAGING_PAGE_WRITETHROUGH    = 0x08,
    PAGING_PAGE_CACHEDISABLE    = 0x10,
    PAGING_PAGE_ACCESSED        = 0x20,
    PAGING_PAGE_PAGESIZE_4M     = 0x40,
    PAGING_PAGE_DIRTY           = 0x40,
    PAGING_PAGE_GLOBAL          = 0x80
};

extern void paging_change_directory(uintptr_t directoryPhysicalAddr);
extern void paging_flush_tlb();
extern void paging_flush_tlb_address(uintptr_t address);
extern void paging_map(uintptr_t virt, uint64_t phys, bool kernel, bool writeable);
extern void paging_unmap(uintptr_t virtual);
extern bool paging_get_phys(uintptr_t virtual, uint64_t *physOut);

extern void paging_map_region(uintptr_t startAddress, uintptr_t endAddress, bool kernel, bool writeable);
extern void paging_map_region_phys(uintptr_t startAddress, uintptr_t endAddress, uint64_t startPhys, bool kernel, bool writeable);
extern void paging_unmap_region(uintptr_t startAddress, uintptr_t endAddress);
extern void paging_unmap_region_phys(uintptr_t startAddress, uintptr_t endAddress);

extern void paging_init();

#endif
