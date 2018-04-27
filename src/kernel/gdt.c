#include <main.h>
#include <kprint.h>
#include <kernel/gdt.h>
#include <kernel/tss.h>

// Function to load GDT, implemented in gdt.asm.
extern void _gdt_load(void);
extern void _gdt_flush_tss(void);

// Represents the 32-bit GDT and pointer to it. This is used as
// the system GDT on 32-bit systems and when starting up other processors,
gdt_entry_t gdt32[GDT32_ENTRIES];
gdt_ptr_t gdt32Ptr;

// TSS.
tss_entry_t tss32 = { };

#ifdef X86_64
// Represents the 64-bit GDT and pointer to it. Only used in 64-bit mode.
gdt_entry_t gdt64[GDT64_ENTRIES];
gdt_ptr_t gdt64Ptr;
#endif

// Sets up a GDT descriptor.
static void gdt_set_descriptor(gdt_entry_t gdt[], int32_t num, bool code, bool userMode) {
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
    gdt[num].IsLimit4K = true;

#ifdef X86_64
    gdt[num].Is64Bits = code;
    gdt[num].Is32Bits = !code;
#else
    gdt[num].Is64Bits = false;
    gdt[num].Is32Bits = true;
#endif
}

static void gdt_set_tss(gdt_entry_t gdt[], int32_t num, uint32_t base, uint32_t size) {
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
    gdt[num].IsLimit4K = false;

#ifdef X86_64
    gdt[num].Is64Bits = code;
    gdt[num].Is32Bits = !code;
#else
    gdt[num].Is64Bits = false;
    gdt[num].Is32Bits = true;
#endif
}

// Loads the GDT.
void gdt_load(void) {
    // Load the GDT into the processor.
       _gdt_load();
    kprintf("32-bit GDT: Loaded at 0x%p.\n", &gdt32Ptr);

#ifdef X86_64
    kprintf("64-bit GDT: Loaded at 0x%p.\n", &gdt64Ptr);
#endif
}

// Initializes the GDT.
void gdt_init(void) {
    kprintf("GDT: Initializing...\n");

    // Set up the 32-bit GDT pointer and limit.
    gdt32Ptr.Limit = (sizeof(gdt_entry_t) * GDT32_ENTRIES) - 1;
    gdt32Ptr.Base = (uintptr_t)&gdt32;

    // Set null segment for 32-bit GDT.
    gdt32[0] = (gdt_entry_t) { };

    // Set code and data segment for kernel for 32-bit GDT.
    gdt_set_descriptor(gdt32, 1, true, false);
    gdt_set_descriptor(gdt32, 2, false, false);

    // Set code and data segement for ring 3 (user mode) for 32-bit GDT.
    gdt_set_descriptor(gdt32, 3, true, true);
    gdt_set_descriptor(gdt32, 4, false, true);
    
    // Create TSS.
    tss32.Ss0 = 0x10;
    tss32.Ss = 0x13;
    tss32.Cs = 0x0B;
    tss32.Es = 0x13;
    tss32.Ds = 0x13;
    tss32.Fs = 0x13;
    tss32.Gs = 0x13;
    gdt_set_tss(gdt32, 5, (uint32_t)&tss32, sizeof(tss_entry_t));

#ifdef X86_64
    // Set up the 64-bit GDT pointer and limit.
    gdt64Ptr.limit = (sizeof(gdt_entry_t) * GDT64_ENTRIES) - 1;
    gdt64Ptr.base = (uintptr_t)&gdt64;

    // Add descriptors to 64-bit GDT.
    gdt_set_descriptor(gdt64, 0, 0, 0, 0, 0);                			// Null segment.
    gdt_set_descriptor(gdt64, 1, 0, 0xFFFFFFFF, 0x9A, 0xAF); 			// Code segment.
    gdt_set_descriptor(gdt64, 2, 0, 0xFFFFFFFF, 0x92, 0xCF); 			// Data segment.
    gdt_set_descriptor(gdt64, 3, 0, 0xFFFFFFFF, 0xFA, 0xAF); 			// User mode code segment.
    gdt_set_descriptor(gdt64, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 			// User mode data segment.

    //gdt_set_descriptor(5, 0x80000, 0x67, 0xE9, 0x00); // User mode data segment.
    //gdt_set_descriptor(6, 0, 0x80, 0x00, 0x00); // User mode data segment.);
#endif


    // Load the GDT.
    gdt_load();
    _gdt_flush_tss();
    kprintf("GDT: Initialized!\n");
}
