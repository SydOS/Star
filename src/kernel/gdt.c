/*
 * File: gdt.c
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
#include <kprint.h>
#include <string.h>
#include <kernel/gdt.h>
#include <kernel/tss.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load(gdt_ptr_t *gdtPtr, uintptr_t offsets);
extern void _gdt_tss_load(uintptr_t offset);

// Represents the 32-bit GDT for the BSP and pointer to it. This is used as
// the system GDT on 32-bit systems and when starting up other processors,
static gdt_entry_t bspGdt32[GDT32_ENTRIES];

#ifdef X86_64
// Represents the 64-bit GDT for the BSP and pointer to it. Only used in 64-bit mode.
static gdt_entry_t bspGdt64[GDT64_ENTRIES];
#endif

// Task state segment. Used for switching from ring 3 tasks.
static tss_t bspTss;

/**
 * Gets the BSP's 32-bit GDT.
 */
gdt_entry_t *gdt_get_bsp32(void) {
    return bspGdt32;
}

#ifdef X86_64
/**
 * Gets the BSP's 64-bit GDT.
 */
gdt_entry_t *gdt_get_bsp64(void) {
    return bspGdt64;
}
#endif

/**
 * Creates an entry in the GDT.
 */
static void gdt_set_descriptor(gdt_entry_t *gdt, uint8_t num, bool code, bool userMode, bool is64Bits) {
    // Set up the descriptor base address.
    gdt[num].BaseLow = 0;
    gdt[num].BaseHigh = 0;

    // Set up the descriptor limits.
    gdt[num].LimitLow = 0xFFFF;
    gdt[num].LimitHigh = 0xF;

    // Set access bits.
    gdt[num].Writeable = true;
    gdt[num].DirectionConforming = false;
    gdt[num].Executable = code;
    gdt[num].DescriptorBit = true;
    gdt[num].Privilege = userMode ? GDT_PRIVILEGE_USER : GDT_PRIVILEGE_KERNEL;
    gdt[num].Present = true;

    // Set flags.
    gdt[num].ReservedZero = 0;
    gdt[num].Is64Bits = is64Bits;
    gdt[num].Is32Bits = !is64Bits;
    gdt[num].IsLimit4K = true;
}

/**
 * Creates the TSS entry in the GDT.
 */
static void gdt_set_tss(gdt_entry_t *gdt, uint8_t num, uintptr_t base, uint32_t size) {
    // Set TSS base address.
    gdt[num].BaseLow = (base & 0xFFFFFF);
    gdt[num].BaseHigh = (base >> 24) & 0xFF;

    // Set TSS size.
    gdt[num].LimitLow = (size & 0xFFFF);
    gdt[num].LimitHigh = (size >> 16) & 0x0F;

    // Set access bits.
    gdt[num].Accessed = true;
    gdt[num].Writeable = false;
    gdt[num].DirectionConforming = false;
    gdt[num].Executable = true;
    gdt[num].DescriptorBit = false;
    gdt[num].Privilege = GDT_PRIVILEGE_USER;
    gdt[num].Present = true;

    // Set flags.
    gdt[num].ReservedZero = 0;
    gdt[num].Is64Bits = false;
    gdt[num].Is32Bits = false;
    gdt[num].IsLimit4K = false;

#ifdef X86_64
    // Set high 32 bits of TSS address.
    gdt[num+1].LimitLow = (base >> 32) & 0xFFFF;
    gdt[num+1].BaseLow = (base >> 48) & 0xFFFF;

    // Other address fields are zero.
    gdt[num+1].BaseHigh = 0;
    gdt[num+1].LimitHigh = 0;

    // Set access bits.
    gdt[num+1].Accessed = false;
    gdt[num+1].Writeable = false;
    gdt[num+1].DirectionConforming = false;
    gdt[num+1].Executable = false;
    gdt[num+1].DescriptorBit = false;
    gdt[num+1].Privilege = 0;
    gdt[num+1].Present = false;

    // Set flags.
    gdt[num+1].ReservedZero = 0;
    gdt[num+1].Is64Bits = false;
    gdt[num+1].Is32Bits = false;
    gdt[num+1].IsLimit4K = false;
#endif
}

/**
 * Sets the kernel ESP in the TSS.
 */
void gdt_tss_set_kernel_stack(tss_t *tss, uintptr_t stack) {
    // If TSS specified is nothing, use the BSP TSS.
    if (tss == NULL)
        tss = &bspTss;

    // Set stack.
#ifdef X86_64
    tss->RSP0 = stack;
#else
    tss->SS0 = GDT_KERNEL_DATA_OFFSET;
    tss->ESP0 = stack;
#endif
}

/**
 * Gets the kernel ESP in the TSS.
 */
uintptr_t gdt_tss_get_kernel_stack(tss_t *tss) {
    // If TSS specified is nothing, use the BSP TSS.
    if (tss == NULL)
        tss = &bspTss;

    // Return stack.
#ifdef X86_64
    // 64-bit code here.
    return tss->RSP0;
#else
    return tss->ESP0;
#endif
}

/**
 * Creates a GDT pointer.
 * @param gdt       The GDT.
 * @param entries   The number of entries in the GDT.
 */
gdt_ptr_t gdt_create_ptr(gdt_entry_t *gdt, uint8_t entries) {
    return (gdt_ptr_t) { (sizeof(gdt_entry_t) * entries) - 1, gdt };
}

/**
 * Loads a GDT.
 * @param gdt       The GDT.
 * @param entries   The number of entries in the GDT.
 */
void gdt_load(gdt_entry_t *gdt, uint8_t entries) {
    // Load the GDT into the processor.
    gdt_ptr_t gdtPtr = gdt_create_ptr(gdt, entries);
    _gdt_load(&gdtPtr, GDT_KERNEL_DATA_OFFSET);
    kprintf("\e[31mGDT: Loaded GDT at 0x%p.\e[0m\n", gdt);
}

/**
 * Loads the TSS.
 */
void gdt_tss_load(tss_t *tss) {
    // Load the TSS into the processor.
    _gdt_tss_load(GDT_TSS_OFFSET);
    kprintf("\e[31mGDT: Loaded TSS at 0x%p.\e[0m\n", tss);
}

/**
 * Gets the GDT for the currently executing processor.
 */
gdt_entry_t *gdt_get(void) {
    // Get GDT pointer.
    gdt_ptr_t gdtPtr = { };
    asm volatile ("sgdt %0" : "=g"(gdtPtr));

    // Return GDT.
    return gdtPtr.Table;
}

/**
 * Gets the TSS for the currently executing processor.
 */
tss_t *gdt_tss_get(void) {
    // Get current GDT.
    gdt_entry_t *gdt = gdt_get();

    // Get TSS offset.
    uint16_t tssOffset = 0;
    asm volatile ("str %%ax" : "=a"(tssOffset));
    uint8_t tssIndex = tssOffset / sizeof(gdt_entry_t);

    // Get TSS address.
    uintptr_t tssAddress = ((uint32_t)(gdt[tssIndex].BaseLow)) | ((uint32_t)(gdt[tssIndex].BaseHigh) << 24);

#ifdef X86_64
    tssAddress |= ((uint64_t)(gdt[tssIndex+1].LimitLow) << 32) | ((uint64_t)(gdt[tssIndex+1].BaseLow) << 48);
#endif

    // Return TSS pointer.
    return (tss_t*)tssAddress;
}

/**
 * Fills the specified GDT.
 * @param gdt       The pointer to the GDT.
 * @param is64Bits  Whether or not the GDT is 64-bit.
 * @param tss       The pointer to a TSS, if any.
 */
void gdt_fill(gdt_entry_t *gdt, bool is64Bits, tss_t *tss) {
    // Set null segment.
    gdt[GDT_NULL_INDEX] = (gdt_entry_t) { };

    // Set code and data segment for kernel.
    gdt_set_descriptor(gdt, GDT_KERNEL_CODE_INDEX, true, false, is64Bits);
    gdt_set_descriptor(gdt, GDT_KERNEL_DATA_INDEX, false, false, false);

    // Set code and data segment for ring 3 (user mode).
    gdt_set_descriptor(gdt, GDT_USER_CODE_INDEX, true, true, is64Bits);
    gdt_set_descriptor(gdt, GDT_USER_DATA_INDEX, false, true, false);

    // If a TSS was specified, add it too.
    if (tss != NULL)
        gdt_set_tss(gdt, GDT_TSS_INDEX, (uintptr_t)tss, sizeof(tss_t));
}

/**
 * Initializes the BSP GDT and TSS.
 */
void gdt_init_bsp(void) {
    // Zero out TSS.
    kprintf("\e[31mGDT: Initializing TSS at 0x%p...\n", &bspTss);
    memset(&bspTss, 0, sizeof(tss_t));

#ifdef X86_64
    // Set initial kernel (ring 0) stack pointer.
    bspTss.RSP0 = 0;
#else
    // Set initial kernel (ring 0) info.
    bspTss.SS0 = GDT_KERNEL_DATA_OFFSET;
    bspTss.ESP0 = 0;
#endif

    // Set up the 32-bit GDT.
    kprintf("GDT: Initializing 32-bit GDT at 0x%p...\n", bspGdt32);
#ifdef X86_64
    gdt_fill(bspGdt32, false, NULL);

    // Set up the 64-bit GDT.
    kprintf("GDT: Initializing 64-bit GDT at 0x%p...\n", bspGdt64);   
    gdt_fill(bspGdt64, true, &bspTss);

    // Load 64-bit GDT.
    gdt_load(bspGdt64, GDT64_ENTRIES);
#else
    gdt_fill(bspGdt32, false, &bspTss);

    // Load 32-bit GDT.
    gdt_load(bspGdt32, GDT32_ENTRIES);
#endif
    
    // Load TSS.
    gdt_tss_load(&bspTss);
    kprintf("\e[31mGDT: Initialized!\e[0m\n");
}
