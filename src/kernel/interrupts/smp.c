#include <main.h>
#include <kprint.h>
#include <io.h>
#include <string.h>
#include <tools.h>
#include <kernel/interrupts/smp.h>

#include <kernel/gdt.h>
#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>

// https://wiki.osdev.org/SMP
// http://ethv.net/workshops/osdev/notes/notes-5

extern uintptr_t _ap_bootstrap_init;
extern uintptr_t _ap_bootstrap_end;
extern gdt_ptr_t gdt32Ptr;

#ifdef X86_64
extern gdt_ptr_t gdt64Ptr;
#endif

static bool smpInitialized;

// Processor counts.
uint32_t cpuCount;
uint8_t *cpus;

// Array holding the address of stack for each AP.
uintptr_t *apStacks;

// Bitmap for indicating which processors are initialized, used during initial startup.
static volatile uint64_t ap_map;

uint32_t ap_get_stack(uint32_t apicId) {
    uint32_t index = 0;
    while (index < cpuCount && cpus[index] != apicId)
        index++;

    if (index >= cpuCount)
        panic("SMP: Failed to find stack for processor LAPIC %u!\n", apicId);
    return apStacks[index];
}

void ap_main() {
    // Get processor ID.
    uint32_t cpu = lapic_id();

    kprintf("Hi from a core!\n");
    kprintf("I should be core %u\n", cpu);

    // Processor is initialized.
    ap_map |= (1 << lapic_id());

    // Load existing GDT and IDT into processor.
    gdt_load();
    idt_load();
    //lapic_setup();

    // Enable interrupts.
    //asm volatile ("sti");
    kprintf("CPU%u: INTERRUPTS ENABLED.\n", cpu);
    while (true) {
        sleep(2000);
        kprintf("Tick tock, I'm CPU %d!\n", cpu);
    }
}

void smp_setup_stacks() {
    // Allocate space for stack list.
    apStacks = kheap_alloc(sizeof(uintptr_t*) * cpuCount);

    // Allocate space for each processor's stack.
    for (uint32_t cpu = 0; cpu < cpuCount; cpu++) {
        // Don't need a stack for the BSP.
        if (cpu == lapic_id()) {
            apStacks[cpu] = 0;
            continue;
        }

        // Allocate space for stack.
        apStacks[cpu] = (uintptr_t)kheap_alloc(SMP_AP_STACK_SIZE);

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
    kprintf("SMP:     Bootstrap start: 0x%X, size: %u bytes\n", apStart, apSize);

    // Ensure AP bootstrap is within a page.
    if (apSize > PAGE_SIZE_4K)
        panic("SMP: AP bootstrap code bigger than a 4KB page.\n");
    
    // Identity map low memory and kernel.
    for (page_t page = 0; page <= memInfo.kernelEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K)
        paging_map_virtual_to_phys(page, page);
    
    // Copy root paging structure and GDT into low memory.
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_PAGING_ADDRESS), (void*)&memInfo.kernelPageDirectory, sizeof(memInfo.kernelPageDirectory));
    memset((void*)(memInfo.kernelVirtualOffset + SMP_PAGING_PAE_ADDRESS), memInfo.paeEnabled ? 1 : 0, sizeof(uint32_t));
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_GDT32_ADDRESS), (void*)&gdt32Ptr, sizeof(gdt_ptr_t));

#ifdef X86_64
    // Copy 64-bit GDT.
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_GDT64_ADDRESS), (void*)&gdt64Ptr, sizeof(gdt_ptr_t));
#endif

    // Copy AP bootstrap code into low memory.
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_AP_BOOTSTRAP_ADDRESS), (void*)apStart, apSize);
}

void smp_destroy_apboot() {
    // Remove identity mapping.
    for (page_t page = 0; page <= memInfo.kernelEnd - memInfo.kernelVirtualOffset; page += PAGE_SIZE_4K)
        paging_map_virtual_to_phys(page, 0);
}

void smp_init() {
    kprintf("SMP: Initializing...\n");

    // Only initialize once.
    if (smpInitialized)
        panic("SMP: Attempting to initialize multiple times!\n");

    // Search for LAPICs (processors) in ACPI.
    cpuCount = 0;
    kprintf("SMP: Looking for processors...\n");
    acpi_madt_entry_local_apic_t *cpu = (acpi_madt_entry_local_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_LOCAL_APIC, 8, 0);
    while (cpu != NULL) {
        kprintf("SMP:     Found processor %u (APIC 0x%X, %s)!\n", cpu->acpiProcessorId, cpu->apicId, (cpu->flags & ACPI_MADT_ENTRY_LOCAL_APIC_ENABLED) ? "enabled" : "disabled");

        // Add to count.
        if (cpu->flags & ACPI_MADT_ENTRY_LOCAL_APIC_ENABLED)
            cpuCount++;
        cpu = (acpi_madt_entry_local_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_LOCAL_APIC, 8, ((uintptr_t)cpu) + 1);
    }

    // If none or only one CPU was found in ACPI, no need to further intialize SMP.
    if (cpuCount <= 1) {
        kprintf("SMP: More than one usable processor was not found in the ACPI. Aborting.\n");
        smpInitialized = false;
        return;
    }

    // Allocate space for CPU to LAPIC ID mapping array.
    cpus = kheap_alloc(sizeof(uint8_t) * cpuCount);

    // Search for processors again, this time saving the APIC IDs.
    cpu = (acpi_madt_entry_local_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_LOCAL_APIC, 8, 0);
    uint32_t currentCpu = 0;
    while (cpu != NULL && currentCpu < cpuCount) {
        if (cpu->flags & ACPI_MADT_ENTRY_LOCAL_APIC_ENABLED) {
            cpus[currentCpu] = cpu->apicId;
            currentCpu++;
        }
        cpu = (acpi_madt_entry_local_apic_t*)acpi_search_madt(ACPI_MADT_STRUCT_LOCAL_APIC, 8, ((uintptr_t)cpu) + 1);
    }

    kprintf("SMP: Waiting two seconds for verification...\n");
    sleep(2000);

    // Initialize boot code and stacks for APs.
    kprintf("SMP: Initializing %u processors...\n", cpuCount);
    smp_setup_apboot();
    smp_setup_stacks();

    // Initialize each processor.
    for (uint32_t cpu = 0; cpu < cpuCount; cpu++) {
        // Don't need to initialize the boot processor (BSP).
        if (cpus[cpu] == lapic_id())
            continue;

        // Send INIT command to processor (AP).
        kprintf("SMP: Sending INIT to LAPIC %u on processor %u\n", cpus[cpu], cpu);
        lapic_send_init(cpus[cpu]);

        // Wait 10ms.
        sleep(10);

        // Send STARTUP command to processor (AP).
        kprintf("SMP: Sending STARTUP to LAPIC %u on processor %u\n", cpus[cpu], cpu);
        lapic_send_startup(cpus[cpu], SMP_AP_BOOTSTRAP_ADDRESS / PAGE_SIZE_4K);

        // Wait for processor to come up.
        while (!(ap_map & (1 << cpus[cpu])));
    }

    // Destroy AP boot code.
    smp_destroy_apboot();
    kprintf("SMP: Initialized!\n");
}
