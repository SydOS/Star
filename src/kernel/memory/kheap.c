#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>

#include <kernel/memory/kheap.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/pmm.h>

// Based on code from https://github.com/CCareaga/heap_allocator. Licensed under the MIT.

static size_t currentKernelHeapSize;
static kheap_bin_t bins[KHEAP_BIN_COUNT];

static void kheap_add_node(uint32_t binIndex, kheap_node_t *node) {
    // Null out the sibling nodes.
    node->nextNode = NULL;
    node->previousNode = NULL;

    // Ensure bin header is valid.
    if (bins[binIndex].header == NULL) {
        bins[binIndex].header = node;
        return;
    }

    // Pointers to next and previous while iterating.
    kheap_node_t *current = bins[binIndex].header;
    kheap_node_t *previous = NULL;

    // Iterate through until we get to the end or find a node.
    while (current != NULL && current->size <= node->size) {
        previous = current;
        current = current->nextNode;
    }

    // Did we reach the end of the list?
    if (current == NULL) {
        previous->nextNode = node;
        node->previousNode = previous;
    }
    else {
        // Are we in the middle of the list?
        if (previous != NULL) {
            node->nextNode = current;
            previous->nextNode = node;

            node->previousNode = previous;
            current->previousNode = node;
        }
        else { // Header is the only element.
            node->nextNode = bins[binIndex].header;
            bins[binIndex].header->previousNode = node;
            bins[binIndex].header = node;
        }
    }
}

static void kheap_remove_node(uint32_t binIndex, kheap_node_t *node) {
    if (bins[binIndex].header == NULL)
        return;
    if (bins[binIndex].header == node) {
        bins[binIndex].header = bins[binIndex].header->nextNode;
        return;
    }

    // Search for node.
    kheap_node_t *tempNode = bins[binIndex].header->nextNode;
    while (tempNode != NULL) {
        // Did we find it?
        if (tempNode == node) {
            // Last item.
            if (tempNode->nextNode == NULL) {
                tempNode->previousNode->nextNode = NULL;
            }
            else { // Middle item.
                tempNode->previousNode->nextNode = tempNode->nextNode;
                tempNode->nextNode->previousNode = tempNode->previousNode;
            }

            return;
        }
        tempNode = tempNode->nextNode;
    }
}

static kheap_node_t *kheap_get_best_fit(uint32_t binIndex, size_t size) {
    // If requested bin is beyond the max, return nothing.
    if (binIndex >= KHEAP_BIN_COUNT)
        return NULL;

    // Ensure list is not empty.
    if (bins[binIndex].header == NULL)
        return NULL;

    // Attempt to find a fit.
    kheap_node_t *tempNode = bins[binIndex].header;

    // If node is at 0x0, it's not possible to find a fit.
    if (tempNode == NULL)
        return NULL;

    while (tempNode != NULL) {
        // Did we find a fit?
        if (tempNode->size >= size)
            return tempNode;

        // If next node is at 0x0, don't bother continuing a search.
        if (tempNode->nextNode == NULL)
            break;
        tempNode = tempNode->nextNode;
    }

    // No fit found.
    return NULL;
}

/*static kheap_node_t *kheap_get_last_node(uint32_t binIndex) {
    // Get the last node in list.
    kheap_node_t *node = bins[binIndex].header;
    while (node->nextNode != NULL)
        node = node->nextNode;
    return node;
}*/

static void kheap_dump_bin(uint32_t binIndex) {
    kheap_node_t *node = bins[binIndex].header;

    while (node != NULL) {
        kprintf_nlock("NODE: 0x%X size: %u hole: %s\n", node, node->size, node->hole ? "yes" : "no");
        node = node->nextNode;
    }
}

void kheap_dump_all_bins() {
    for (uint8_t i = 0; i < 9; i++) {
        kprintf_nlock("Bin %u:\n", i);
        kheap_dump_bin(i);
    }
}

static uint32_t kheap_get_bin_index(size_t size) {
    uint32_t index = 0;

    // If size is less than 4, make it 4.
    size = size < 4 ? 4 : size;

    // Calculate log2(size).
    while (size >>= 1)
        index++;
    index -= 2;

    // If size was bigger than 1024, make it 8.
    if (index > 8)
        index = 8;
    return index;
}

static kheap_footer_t *kheap_get_footer(kheap_node_t *node) {
    // Get footer for node.
    return (kheap_footer_t*)((uint8_t*)node + sizeof(kheap_node_t) + node->size);
}

static kheap_node_t *kheap_get_wilderness() {
    kheap_footer_t *wildFooter = (kheap_footer_t*)(KHEAP_START + currentKernelHeapSize - sizeof(kheap_footer_t));
    return wildFooter->header;
}

static void kheap_create_footer(kheap_node_t *header) {
    // Create a footer from the header.
    kheap_footer_t *footer = kheap_get_footer(header);
    footer->header = header;
}

static bool kheap_expand(size_t size) {
    size_t newSize = currentKernelHeapSize + size;

    for (page_t page = currentKernelHeapSize; page < ALIGN_4K(newSize) - PAGE_SIZE_4K; page += PAGE_SIZE_4K) {
        // Pop another page and increase size of heap.
        paging_map(KHEAP_START + page, pmm_pop_frame(), true, true);

        // Increase wilderness size.
        kheap_node_t *wildNode = kheap_get_wilderness();

        wildNode->size += PAGE_SIZE_4K;
        kheap_create_footer(wildNode);   
        currentKernelHeapSize += PAGE_SIZE_4K;
    }

    kprintf_nlock("KHEAP: Heap expanded by 4KB to %u bytes!\n", currentKernelHeapSize);
    return true;
}

static void kheap_contract() {
    // TODO.
}

void *kheap_alloc(size_t size) {
    // Get bin index that the chunk should be in
    uint32_t binIndex = kheap_get_bin_index(size);

    // Try to find a good fitting chunk.
    kheap_node_t *node = kheap_get_best_fit(binIndex, size);

    // If a chunk wasn't found, iterate through the bins until we find one.
    while (node == NULL && binIndex < KHEAP_BIN_COUNT)
        node = kheap_get_best_fit(++binIndex, size);

    // If a chunk still couldn't be found, expand heap.
    if (node == NULL) {
        if (!kheap_expand(size)) {
            kprintf_nlock("KHEAP: Failed to expand heap!\n");
            return NULL;
        }

        // Start at index 0.
        binIndex = 0;

        // Try to find a good fitting chunk.
        node = kheap_get_best_fit(binIndex, size);

        // If a chunk wasn't found, iterate through the bins until we find one.
        while (node == NULL && binIndex < KHEAP_BIN_COUNT)
            node = kheap_get_best_fit(++binIndex, size);
    }

    // If the difference between the found and requested chunks is bigger than overhead, then split the chunk.
    if (node == NULL)
        kheap_dump_all_bins();
    if ((node->size - size) > (KHEAP_OVERHEAD + 4)) {
        // Determine where to split at.
        kheap_node_t *splitNode = (kheap_node_t*)(((uint8_t*)node + KHEAP_OVERHEAD) + size);
        splitNode->size = node->size - size - (KHEAP_OVERHEAD);
        splitNode->hole = true;

        // Create foooter for the split.
        kheap_create_footer(splitNode);

        // Get index for the split chunk, and place in correct bin.
        uint32_t newBinIndex = kheap_get_bin_index(splitNode->size);
        kheap_add_node(newBinIndex, splitNode);

        // Set chunk size and re-make footer.
        node->size = size;
        kheap_create_footer(node);
    }

    // Chunk isn't a hole anymore, so remove it from bin.
    node->hole = false;
    kheap_remove_node(binIndex, node);

    // Check if heap needs to be expanded or contracted.
    kheap_node_t *wildNode = kheap_get_wilderness();
    if (wildNode->size < KHEAP_MIN_WILDERNESS) {
        if (!kheap_expand(PAGE_SIZE_4K))
            return NULL;
    }
    else if (wildNode->size > KHEAP_MAX_WILDERNESS) {
        kheap_contract();
    }

    // As the chunk is in use, clear the next and previous node fields.
    node->previousNode = NULL;
    node->nextNode = NULL;
    return (uint8_t*)node + KHEAP_HEADER_OFFSET;
}

void kheap_free(void *ptr) {
    // Get header of node to free.
    kheap_node_t *header = (kheap_node_t*)((uint8_t*)ptr - KHEAP_HEADER_OFFSET);

    // If the node is also the start of the heap, just put it in the correct bin.
    if (header == (kheap_node_t*)KHEAP_START) {
        header->hole = true;
        kheap_add_node(kheap_get_bin_index(header->size), header);
        return;
    }

    // Get next and previous nodes of heap.
    kheap_node_t *nextNode = (kheap_node_t*)((uint8_t*)kheap_get_footer(header) + sizeof(kheap_footer_t));
    kheap_footer_t *footer = (kheap_footer_t*)((uint8_t*)header - sizeof(kheap_footer_t));
    kheap_node_t *previousNode = footer->header;

    // Is the previous node a hole?
    if (previousNode->hole) {
        // Remove previous node from bin.
        kheap_remove_node(kheap_get_bin_index(previousNode->size), previousNode);

        // Re-calculate size and footer for node.
        previousNode->size += KHEAP_OVERHEAD + header->size;
        kheap_create_footer(previousNode);

        // Previous node is now the header.
        header = previousNode;
    }

    // Is the next node a hole?
    if (nextNode->hole) {
        // Remove next node from bin.
        kheap_remove_node(kheap_get_bin_index(nextNode->size), nextNode);

        // Re-calculate size of header.
        header->size += KHEAP_OVERHEAD + nextNode->size;

        // Clear out metadata from next node.
        kheap_footer_t *oldFooter = kheap_get_footer(nextNode);
        oldFooter->header = 0;
        nextNode->size = 0;
        nextNode->hole = false;

        // Make new footer.
        kheap_create_footer(header);
    }

    // Chunk is now a hole, place it in correct bin.
    header->hole = true;
    kheap_add_node(kheap_get_bin_index(header->size), header);
}

void *kheap_realloc(void *oldPtr, size_t newSize) {
    // Allocate new space using the new size.
    void *newPtr = kheap_alloc(newSize);
    if (newPtr == NULL)
        return NULL;

    // If the old space is a valid pointer, copy data and free old space when done.
    if (oldPtr != NULL) {
        // Get header of existing node.
        kheap_node_t *header = (kheap_node_t*)((uint8_t*)oldPtr - KHEAP_HEADER_OFFSET);

        // Copy data.
        size_t copySize = header->size;
        if (newSize < header->size)
            copySize = newSize;
        memcpy(newPtr, oldPtr, copySize);

        // Free old node.
        kheap_free(oldPtr);
    }

    // Return new node.
    return newPtr;
}

void kheap_init() {
    kprintf("KHEAP: Initializing at 0x%p...\n", KHEAP_START);

    // Start with 4MB heap.
    currentKernelHeapSize = KHEAP_INITIAL_SIZE;
    paging_map_region(KHEAP_START, KHEAP_START + currentKernelHeapSize, true, true);

    // Test heap area.
    kprintf("KHEAP: Testing %uKB of heap memory...\n", currentKernelHeapSize / 1024);
    uint32_t *testBuffer = (uint32_t*)KHEAP_START;
    for (uint32_t i = 0; i < currentKernelHeapSize / sizeof(uint32_t); i++)
        testBuffer[i] = i;

    bool pass = true;
    for (uint32_t i = 0; i < currentKernelHeapSize / sizeof(uint32_t); i++)
        if (testBuffer[i] != i) {
            pass = false;
            break;
        }
    kprintf("KHEAP: Test %s!\n", pass ? "passed" : "failed");
    if (!pass)
        panic("KHEAP: Memory test of heap failed.\n");

    // Create initial region, the "wilderness" chunk, as the heap is one
    // large unallocated chunk to begin with.
    kheap_node_t *initialRegion = (kheap_node_t*)KHEAP_START;
    initialRegion->hole = true;
    initialRegion->size = KHEAP_INITIAL_SIZE - KHEAP_OVERHEAD;

    // Create the footer for the initial region.
    kheap_create_footer(initialRegion);

    // Add the initial region to the correct bin.
    kheap_add_node(kheap_get_bin_index(initialRegion->size), initialRegion);
    kprintf("KHEAP: Kernel heap at 0x%p with a size of %uKB initialized!\n", KHEAP_START, currentKernelHeapSize / 1024);

    // Attempt allocation.
    kprintf("KHEAP: Allocation 1: ");
    uint32_t *test = kheap_alloc(sizeof(uint32_t)*2);
    kprintf("KHEAP: 0x%X\n", test);
    test[1] = 55555;

    kprintf("KHEAP: Allocation 2: ");
    uint32_t *test2 = kheap_alloc(sizeof(uint32_t)*2);
    kprintf("KHEAP: 0x%X\n", test2);
    test2[1] = 55335;

    kprintf("KHEAP: Big allocation: ");
    uint32_t *big = kheap_alloc(sizeof(uint32_t)*1000);
    kprintf("KHEAP: 0x%X, size: %u\n", big, sizeof(uint32_t)*1000);

    for (uint32_t i = 0; i < 1000; i++)
        big[i] = i*2;

    for (uint32_t i = 0; i < 1000; i++) {
        if (big[i] != i*2) {
            kprintf("Error.\n");
            break;
        }
    }


    kprintf("KHEAP: Allocation 4: ");
    uint32_t *test4 = kheap_alloc(sizeof(uint32_t)*2);
    kprintf("KHEAP: 0x%X\n", test4);

    test4[1] = 55335;

    kheap_free(test);
    kheap_free(big);
    kheap_free(test2);
    kheap_free(test4);
    kprintf("KHEAP: Initialized!\n");
}
