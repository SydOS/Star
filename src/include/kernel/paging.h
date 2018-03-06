#ifndef PAGING_H
#define PAGING_H

#include <main.h>
#include <kernel/pmm.h>

// Refer to Chapter 4: Paging of volume 3 of the Intel SDM for more info.

// Sizes.
#define PAGE_SIZE_4K                0x1000
#define PAGE_SIZE_2M		        0x200000
#define PAGE_SIZE_4M		        0x400000
#define PAGE_SIZE_1G		        0x40000000
#define PAGE_DIRECTORY_SIZE         1024
#define PAGE_TABLE_SIZE             1024
#define PAGE_TABLE_ADDRESS_START    ((uint32_t)PAGE_SIZE_4M * (uint32_t)(PAGE_DIRECTORY_SIZE - 1))

// PAE sizes.
#ifndef NO_PAE
#define PAGE_PAE_PDPT_SIZE          4
#define PAGE_PAE_DIRECTORY_SIZE     512
#define PAGE_PAE_TABLE_SIZE         512
#endif

// Mask macros.
#define MASK_PAGE_4K(size)          ((uint32_t)(size) & 0xFFFFF000)     // Get only the page address.
#define MASK_PAGEFLAGS_4K(size)     ((uint32_t)(size) & ~0xFFFFF000)    // Get only the page flags.

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

extern void paging_map_kernel_virtual_to_phys(page_t virt, page_t phys);
extern void paging_init();

#endif
