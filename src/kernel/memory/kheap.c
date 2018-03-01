#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <kernel/paging.h>
#include <kernel/pmm.h>
#include <kernel/kheap.h>

static page_t currentKernelHeapEnd;

void kheap_init() {
    currentKernelHeapEnd = KHEAP_VIRTUAL_ADDRESS_START + PAGE_SIZE_4M;

    for (page_t i = KHEAP_VIRTUAL_ADDRESS_START; i <= currentKernelHeapEnd; i+=PAGE_SIZE_4K)
        paging_map_kernel_virtual_to_phys(i, pmm_pop_page());

    // Test heap area.
    kprintf("Testing %uKB of heap memory...\n", (currentKernelHeapEnd - KHEAP_VIRTUAL_ADDRESS_START) / 1024);
    page_t *testBuffer = (page_t*)KHEAP_VIRTUAL_ADDRESS_START;
    for (page_t i = 0; i < (currentKernelHeapEnd - KHEAP_VIRTUAL_ADDRESS_START) / sizeof(page_t); i++)
        testBuffer[i] = i;

    bool pass = true;
    for (page_t i = 0; i < (currentKernelHeapEnd - KHEAP_VIRTUAL_ADDRESS_START) / sizeof(page_t); i++)
        if (testBuffer[i] != i) {
            pass = false;
            break;
        }
    kprintf("Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("Memory test of virtual address 0x1000 failed.\n");

    kprintf("Kernel heap initialized!\n");
}
