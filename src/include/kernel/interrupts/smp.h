/*
 * File: smp.h
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

#ifndef SMP_H
#define SMP_H

#include <main.h>

#define SMP_PAGING_ADDRESS          0x500
#define SMP_PAGING_PAE_ADDRESS      0x510
#define SMP_GDT32_ADDRESS           0x5A0
#define SMP_GDT64_ADDRESS           0x600
#define SMP_PAGING_PML4             0x7000

#define SMP_AP_BOOTSTRAP_ADDRESS    0xA000

#define SMP_AP_STACK_SIZE           0x4000

// Struct for mapping APIC IDs to a 0-based index.
typedef struct smp_proc_t {
    // Link to next processor.
    struct smp_proc_t *Next;

    // APIC ID and index.
    uint32_t ApicId;
    uint32_t Index;

    // Set once processor is started up.
    bool Started;
} smp_proc_t;

extern uint32_t smp_get_proc_count(void);
extern smp_proc_t *smp_get_proc(uint32_t apicId);
extern void smp_init(void);

#endif
