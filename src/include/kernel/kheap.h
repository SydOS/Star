#ifndef KHEAP_H
#define KHEAP_H

#include <main.h>

#define KHEAP_START         0xD0000000
#define KHEAP_END           0xDFFFFFFF
#define KHEAP_INITIAL_SIZE  (4096*1024)
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

extern void *kheap_alloc(size_t size);
extern void kheap_free(void *ptr);
extern void kheap_init();

#endif
