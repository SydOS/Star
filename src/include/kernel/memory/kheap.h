#ifndef KHEAP_H
#define KHEAP_H

#include <main.h>

#ifdef X86_64
#define KHEAP_START         0xFFFF808000000000
#define KHEAP_END           0xFFFF808FFFFFFFFF
#else
#define KHEAP_START         0xD0000000
#define KHEAP_END           0xDFFFFFFF
#endif
#define KHEAP_INITIAL_SIZE  0x1000
#define KHEAP_MAX_SIZE      (KHEAP_END - KHEAP_START)

struct kheap_node {
    bool hole;
    size_t size;
    struct kheap_node *previousNode;
    struct kheap_node *nextNode;
};
typedef struct kheap_node kheap_node_t;

struct kheap_footer {
    kheap_node_t *header;
};
typedef struct kheap_footer kheap_footer_t;

struct kheap_bin {
    kheap_node_t *header;
    size_t size;
};
typedef struct kheap_bin kheap_bin_t;

#define KHEAP_OVERHEAD              (sizeof(kheap_footer_t) + sizeof(kheap_node_t))
#define KHEAP_HEADER_OFFSET         (sizeof(kheap_node_t))
#define KHEAP_MIN_WILDERNESS        0x2000
#define KHEAP_MAX_WILDERNESS        0x1000000
#define KHEAP_BIN_COUNT             9

extern void *kheap_alloc(size_t size);
extern void kheap_free(void *ptr);
extern void *kheap_realloc(void *oldPtr, size_t newSize);
extern void kheap_init();

#endif
