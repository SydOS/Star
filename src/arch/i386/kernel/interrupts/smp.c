#include <main.h>
#include <kprint.h>
#include <io.h>
#include <string.h>
#include <arch/i386/kernel/smp.h>

#include <arch/i386/kernel/acpi.h>
#include <arch/i386/kernel/lapic.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>

// https://wiki.osdev.org/SMP
// http://ethv.net/workshops/osdev/notes/notes-5

extern uintptr_t _ap_bootstrap_init;
extern uintptr_t _ap_bootstrap_end;

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

    // Copy AP bootstrap code to the bootstrap address in low memory.
    memcpy(memInfo.kernelVirtualOffset + SMP_AP_BOOTSTRAP_ADDRESS, (void*)apStart, apSize);
}

void smp_init() {
    // Get processor count.
    uint32_t cpus = acpi_get_cpu_count();
    kprintf("SMP: Initializing %u processors...\n", cpus);

    // Initialize AP boot code.
    smp_setup_apboot();

    // Initialize each processor.
    for (uint32_t cpu = 0; cpu < cpus; cpu++) {
        // Don't need to initialize the boot processor (BSP).
        if (cpu == lapic_id())
            continue;

        // Send INIT command to processor (AP).
        kprintf("SMP: Sending INIT to LAPIC on processor %u\n", cpu);
        lapic_send_init(cpu);

        for (int i = 0; i < 1000000; i++);

        kprintf("SMP: Sending STARTUP to LAPIC on processor %u\n", cpu);
        lapic_send_startup(cpu, SMP_AP_BOOTSTRAP_ADDRESS / PAGE_SIZE_4K);
        while(true);
    }
}
