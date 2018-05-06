/*
 * File: kheap.h
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
extern void kheap_init(void);

#endif
