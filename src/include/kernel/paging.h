#ifndef PAGING_H
#define PAGING_H

#include <main.h>

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE     1024

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

extern void paging_initialize();

#endif
