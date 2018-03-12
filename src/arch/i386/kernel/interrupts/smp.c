#include <main.h>
#include <kprint.h>
#include <io.h>
#include <string.h>
#include <arch/i386/kernel/smp.h>

#include <arch/i386/kernel/acpi.h>
#include <arch/i386/kernel/gdt.h>
#include <arch/i386/kernel/idt.h>
#include <arch/i386/kernel/lapic.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/kheap.h>

// https://wiki.osdev.org/SMP
// http://ethv.net/workshops/osdev/notes/notes-5

extern uintptr_t _ap_bootstrap_init;
extern uintptr_t _ap_bootstrap_end;
extern gdt_ptr_t gdtPtr;

// Array holding the address of stack for each AP.
uintptr_t *apStacks;

// Bitmap for indicating which processors are initialized, used during initial startup.
static volatile uint64_t ap_map;

void ap_main() {
    // Set stack.
    uintptr_t stack = 0;
    asm volatile ("movl %%ebp, %0" : "=r"(stack));

    kprintf("Hi from a core!\n");
    kprintf("I should be core %u\n", lapic_id());

    // Processor is initialized.
    ap_map |= (1 << lapic_id());

    // Load existing GDT and IDT into processor.
    gdt_load();
    idt_load();
}

void smp_setup_stacks() {
    // Get processor count.
    uint32_t cpus = acpi_get_cpu_count();

    // Allocate space for stack list.
    apStacks = kheap_alloc(sizeof(uintptr_t*) * cpus);

    // Allocate space for each processor's stack.
    for (uint32_t cpu = 0; cpu < cpus; cpu++) {
        // Don't need a stack for the BSP.
        if (cpu == lapic_id()) {
            apStacks[cpu] = 0;
            continue;
        }

        // Allocate space for stack.
        kheap_dump_all_bins();
        apStacks[cpu] = kheap_alloc(SMP_AP_STACK_SIZE);

        // Ensure its a valid address.
        if (apStacks[cpu] == 0)
            panic("SMP: Failed to allocate stack for processor %u\n", cpu);
        kprintf("SMP: Allocated stack for processor %u at 0x%X\n", cpu, apStacks[cpu]);
    }
}

void smp_setup_apboot() {
    // Get start and end of the AP bootstrap code.
    uint32_t apStart = (uint32_t)&_ap_bootstrap_init;
    uint32_t apEnd = (uint32_t)&_ap_bootstrap_end;
    uint32_t apSize = apEnd - apStart;

    kprintf("SMP: Setting up bootstrap code for APs...\n");
    kprintf("SMP: Bootstrap start: 0x%X, size: %u bytes\n", apStart, apSize);

    // Ensure AP bootstrap is within a page.
    if (apSize > PAGE_SIZE_4K)
        panic("SMP: AP bootstrap code bigger than a 4KB page.\n");
    
    // Identity map low memory and kernel.
    for (page_t page = 0; page <= memInfo.kernelEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K)
        paging_map_virtual_to_phys(page, page);
    
    // Copy root paging structure and GDT into low memory.
    memcpy(memInfo.kernelVirtualOffset + SMP_PAGING_ADDRESS, &memInfo.kernelPageDirectory, sizeof(memInfo.kernelPageDirectory));
    memset(memInfo.kernelVirtualOffset + SMP_PAGING_PAE_ADDRESS, memInfo.paeEnabled ? 1 : 0, sizeof(uint32_t));
    memcpy(memInfo.kernelVirtualOffset + SMP_GDT_ADDRESS, &gdtPtr, sizeof(gdt_ptr_t));

    // Copy AP bootstrap code into low memory.
    memcpy(memInfo.kernelVirtualOffset + SMP_AP_BOOTSTRAP_ADDRESS, (void*)apStart, apSize);
}

void smp_destroy_apboot() {
    // Remove identity mapping.
    for (page_t page = 0; page <= memInfo.kernelEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K)
        paging_map_virtual_to_phys(page, 0);
}

void smp_init() {
    // Get processor count.
    uint32_t cpus = acpi_get_cpu_count();
    kprintf("SMP: Initializing %u processors...\n", cpus);

    // Initialize AP boot code.
    smp_setup_apboot();

    // Allocate stacks for APs.
    smp_setup_stacks();

    // Initialize each processor.
    for (uint32_t cpu = 0; cpu < cpus; cpu++) {
        // Don't need to initialize the boot processor (BSP).
        if (cpu == lapic_id())
            continue;

        // Send INIT command to processor (AP).
        kprintf("SMP: Sending INIT to LAPIC on processor %u\n", cpu);
        lapic_send_init(cpu);

        for (int i = 0; i < 1000000; i++);

        // Send STARTUP command to processor (AP).
        kprintf("SMP: Sending STARTUP to LAPIC on processor %u\n", cpu);
        lapic_send_startup(cpu, SMP_AP_BOOTSTRAP_ADDRESS / PAGE_SIZE_4K);
        
        // Wait for processor to come up.
        while (!(ap_map & (1 << cpu)));
    }

    // Destroy AP boot code.
    smp_destroy_apboot();
    kprintf("SMP: Initialized!\n");
    //while(true);
}
