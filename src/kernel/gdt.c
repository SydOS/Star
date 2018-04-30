#include <main.h>
#include <kprint.h>
#include <kernel/gdt.h>
#include <kernel/tss.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load(gdt_ptr_t *gdtPtr, uintptr_t offsets);
extern void _gdt_tss_load(uintptr_t offset);

// Represents the 32-bit GDT and pointer to it. This is used as
// the system GDT on 32-bit systems and when starting up other processors,
gdt_entry_t gdt32[GDT32_ENTRIES];
gdt_ptr_t gdt32Ptr;

#ifdef X86_64
// Represents the 64-bit GDT and pointer to it. Only used in 64-bit mode.
gdt_entry_t gdt64[GDT64_ENTRIES];
gdt_ptr_t gdt64Ptr;
#endif

// Task state segment. Used for switching from ring 3 tasks.
static tss_t tss;

/**
 * Creates an entry in the GDT.
 */
static void gdt_set_descriptor(gdt_entry_t gdt[], uint8_t num, bool code, bool userMode, bool Is64Bits) {
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
    gdt[num].Is64Bits = Is64Bits;
    gdt[num].Is32Bits = !Is64Bits;
    gdt[num].IsLimit4K = true;
}

/**
 * Creates the TSS entry in the GDT.
 */
static void gdt_set_tss(gdt_entry_t gdt[], uint8_t num, uintptr_t base, uint32_t size) {
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
void gdt_tss_set_kernel_stack(uintptr_t stack) {
#ifdef X86_64
    // 64-bit code here.
    tss.RSP0 = stack;
#else
    tss.ESP0 = stack;
#endif
}

/**
 * Loads the GDT.
 */
void gdt_load(void) {
    // Load the GDT into the processor.
#ifdef X86_64
    _gdt_load(&gdt64Ptr, GDT_KERNEL_DATA_OFFSET);
    kprintf("GDT: Loaded 64-bit GDT at 0x%p.\n", &gdt64);
#else
    _gdt_load(&gdt32Ptr, GDT_KERNEL_DATA_OFFSET);
    kprintf("GDT: Loaded 32-bit GDT at 0x%p.\n", &gdt32);
#endif
}

/**
 * Loads the TSS.
 */
void gdt_tss_load(void) {
    // Load the TSS into the processor.
    _gdt_tss_load(GDT_TSS_OFFSET);
    kprintf("GDT: Loaded TSS at 0x%p.\n", &tss);
}

/**
 * Initializes the GDT and TSS
 */
void gdt_init(void) {
    // Set up the 32-bit GDT pointer and limit.
    kprintf("GDT: Initializing 32-bit GDT at 0x%p...\n", &gdt32);
    gdt32Ptr.Limit = (sizeof(gdt_entry_t) * GDT32_ENTRIES) - 1;
    gdt32Ptr.Base = (uintptr_t)&gdt32;

    // Set null segment for 32-bit GDT.
    gdt32[GDT_NULL_INDEX] = (gdt_entry_t) { };

    // Set code and data segment for kernel for 32-bit GDT.
    gdt_set_descriptor(gdt32, GDT_KERNEL_CODE_INDEX, true, false, false);
    gdt_set_descriptor(gdt32, GDT_KERNEL_DATA_INDEX, false, false, false);

    // Set data and code segement for ring 3 (user mode) for 32-bit GDT.
    gdt_set_descriptor(gdt32, GDT_USER_DATA_INDEX, false, true, false);
    gdt_set_descriptor(gdt32, GDT_USER_CODE_INDEX, true, true, false);

#ifdef X86_64
    // Set up the 64-bit GDT pointer and limit.
    kprintf("GDT: Initializing 64-bit GDT at 0x%p...\n", &gdt64);
    gdt64Ptr.Limit = (sizeof(gdt_entry_t) * GDT64_ENTRIES) - 1;
    gdt64Ptr.Base = (uintptr_t)&gdt64;

    // Set null segment for 64-bit GDT.
    gdt64[GDT_NULL_INDEX] = (gdt_entry_t) { };

    // Set code and data segment for kernel for 64-bit GDT.
    gdt_set_descriptor(gdt64, GDT_KERNEL_CODE_INDEX, true, false, true);
    gdt_set_descriptor(gdt64, GDT_KERNEL_DATA_INDEX, false, false, false);

    // Set data and code segement for ring 3 (user mode) for 64-bit GDT.
    gdt_set_descriptor(gdt64, GDT_USER_DATA_INDEX, false, true, false);
    gdt_set_descriptor(gdt64, GDT_USER_CODE_INDEX, true, true, true);
#endif

    // Zero out TSS.
    kprintf("GDT: Initializing TSS at 0x%p...\n", &tss);
    memset(&tss, 0, sizeof(tss_t));

#ifdef X86_64
    // Set initial kernel (ring 0) stack pointer.
    tss.RSP0 = 0;

    // Add TSS to GDT.
    gdt_set_tss(gdt64, GDT_TSS_INDEX, (uintptr_t)&tss, sizeof(tss_t));
#else
    // Set initial kernel (ring 0) info.
    tss.SS0 = GDT_KERNEL_DATA_OFFSET;
    tss.ESP0 = 0;

    // Add TSS to GDT.
    gdt_set_tss(gdt32, GDT_TSS_INDEX, (uintptr_t)&tss, sizeof(tss_t));
#endif

    // Load the GDT.
    gdt_load();
    gdt_tss_load();
    kprintf("GDT: Initialized!\n");
}
