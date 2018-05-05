#include <main.h>
#include <kprint.h>
#include <io.h>
#include <string.h>
#include <tools.h>
#include <kernel/interrupts/smp.h>

#include <acpi.h>
#include <kernel/gdt.h>
#include <kernel/acpi/acpi.h>
#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/ioapic.h>
#include <kernel/interrupts/lapic.h>
#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/paging.h>
#include <kernel/tasking.h>

// https://wiki.osdev.org/SMP
// http://ethv.net/workshops/osdev/notes/notes-5

extern uintptr_t _ap_bootstrap_init;
extern uintptr_t _ap_bootstrap_end;

static bool smpInitialized;

// List of processors.
static uint32_t procCount = 1;
static smp_proc_t *processors = NULL;

// Array holding the address of stack for each AP.
uintptr_t *apStacks;

uint32_t smp_get_proc_count(void) {
    return procCount;
}

smp_proc_t *smp_get_proc(uint32_t apicId) {
    // Search for specified APIC ID and return the processor object..
    smp_proc_t *currentProc = processors;
    while (currentProc != NULL) {
        if (currentProc->ApicId == apicId)
            return currentProc;
        currentProc = currentProc->Next;
    }

    // Couldn't find it.
    return NULL;
}

uint32_t smp_ap_get_stack(uint32_t apicId) {
    smp_proc_t *proc = smp_get_proc(apicId);

    if (proc == NULL)
        panic("SMP: Failed to find stack for processor LAPIC %u!\n", apicId);
    return apStacks[proc->Index];
}

static bool test(irq_regs_t *regs, uint8_t irqNum, uint32_t procIndex) {
    // Change tasks every 5ms.
	if (timer_ticks() % 5 == 0)
		tasking_tick(regs, procIndex);
	return true;
}

void smp_ap_main(void) {
    // Reload paging directory.
    paging_change_directory(memInfo.kernelPageDirectory);

    // Get processor.
    smp_proc_t *proc = smp_get_proc(lapic_id());
    kprintf("Hi from core %u (APIC %u)!\n", proc->Index, proc->ApicId);

    // Processor is initialized, so mark it as such which signals the BSP to continue.
    proc->Started = true;

    // Create TSS for this process.
    tss_t *tss = (tss_t*)kheap_alloc(sizeof(tss_t));
    memset(tss, 0, sizeof(tss_t));

    // Create new GDT.
    gdt_entry_t *gdt = (gdt_entry_t*)kheap_alloc(GDT64_SIZE);
    gdt_fill(gdt, true, tss);

    // Load new GDT and TSS.
    gdt_load(gdt, GDT64_ENTRIES);
    gdt_tss_load(tss);

    // Initialize interrupts.
    interrupts_init_ap();
    lapic_setup();

    // Start LAPIC timer.
    lapic_timer_start(lapic_timer_get_rate());

    // Install handler for IRQ0 to handle task switching.
    irqs_install_handler(IRQ_TIMER, test);
    
    // Initialize and start tasking.
    tasking_init_ap();

    // We should never get here.
	panic("SMP: Tasking failed to start on core %u!\n", proc->Index);
}

static void smp_setup_stacks(void) {
    // Allocate space for stack list.
    apStacks = kheap_alloc(sizeof(uintptr_t*) * procCount);

    // Allocate space for each processor's stack.
    for (uint32_t cpu = 0; cpu < procCount; cpu++) {
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

static void smp_setup_apboot(void) {
    // Get start and end of the AP bootstrap code.
    uintptr_t apStart = (uintptr_t)&_ap_bootstrap_init;
    uintptr_t apEnd = (uintptr_t)&_ap_bootstrap_end;
    uint32_t apSize = apEnd - apStart;

    kprintf("SMP: Setting up bootstrap code for APs...\n");
    kprintf("SMP:     Bootstrap start: 0x%p, size: %u bytes\n", apStart, apSize);

    // Ensure AP bootstrap is within a page.
    if (apSize > PAGE_SIZE_4K)
        panic("SMP: AP bootstrap code bigger than a 4KB page.\n");
    
    // Identity map low memory and kernel.
    paging_map_region_phys(0x0, ALIGN_4K_64BIT(memInfo.kernelEnd - memInfo.kernelVirtualOffset), 0x0, true, true);
    
    // Copy BSP's 32-bit GDT pointer into low memory.
    gdt_ptr_t gdtPtr32 = gdt_create_ptr(gdt_get_bsp32(), GDT32_ENTRIES);
    memcpy((void*)((uintptr_t)(memInfo.kernelVirtualOffset + SMP_GDT32_ADDRESS)), (void*)&gdtPtr32, sizeof(gdt_ptr_t));

#ifdef X86_64
    // Copy BSP's 64-bit GDT pointer and PML4 table into low memory.
    gdt_ptr_t gdtPtr64 = gdt_create_ptr(gdt_get_bsp64(), GDT64_ENTRIES);
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_PAGING_PML4), (void*)PAGE_LONG_PML4_ADDRESS, PAGE_SIZE_4K);
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_GDT64_ADDRESS), (void*)&gdtPtr64, sizeof(gdt_ptr_t));
#else
    // Copy root paging structure address into low memory.
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_PAGING_ADDRESS), (void*)&memInfo.kernelPageDirectory, sizeof(memInfo.kernelPageDirectory));
    memset((void*)(memInfo.kernelVirtualOffset + SMP_PAGING_PAE_ADDRESS), memInfo.paeEnabled ? 1 : 0, sizeof(uint32_t));
#endif

    // Copy AP bootstrap code into low memory.
    memcpy((void*)(memInfo.kernelVirtualOffset + SMP_AP_BOOTSTRAP_ADDRESS), (void*)apStart, apSize);
}

static void smp_destroy_apboot(void) {
    // Remove identity mapping.
    paging_unmap_region_phys(0x0, ALIGN_4K_64BIT(memInfo.kernelEnd - memInfo.kernelVirtualOffset));
}

void smp_init(void) {
    // Only initialize once.
    kprintf("SMP: Initializing...\n");
    if (smpInitialized)
        panic("SMP: Attempting to initialize multiple times!\n");

    // Ensure SMP can be supported.
    if (!acpi_supported() || !ioapic_supported()) {
        kprintf("SMP: ACPI or the I/O APIC is not enabled! Aborting.\n");
        return;
    }

    // Search for LAPICs (processors) in ACPI.
    procCount = 0;
    kprintf("SMP: Looking for processors...\n");
    ACPI_MADT_LOCAL_APIC *acpiCpu = (ACPI_MADT_LOCAL_APIC*)acpi_search_madt(ACPI_MADT_TYPE_LOCAL_APIC, 8, 0);
    while (acpiCpu != NULL) {
        kprintf("SMP:     Found processor %u (APIC 0x%X, %s)!\n", acpiCpu->ProcessorId, acpiCpu->Id, (acpiCpu->LapicFlags & ACPI_MADT_ENABLED) ? "enabled" : "disabled");

        // Add to count.
        if (acpiCpu->LapicFlags & ACPI_MADT_ENABLED)
            procCount++;
        acpiCpu = (ACPI_MADT_LOCAL_APIC*)acpi_search_madt(ACPI_MADT_TYPE_LOCAL_APIC, 8, ((uintptr_t)acpiCpu) + 1);
    }

    // If none or only one CPU was found in ACPI, no need to further intialize SMP.
    if (procCount <= 1) {
        kprintf("SMP: More than one usable processor was not found in the ACPI. Aborting.\n");
        smpInitialized = false;
        procCount = 1;
        return;
    }

    // Search for processors again, this time saving the APIC IDs.
    acpiCpu = (ACPI_MADT_LOCAL_APIC*)acpi_search_madt(ACPI_MADT_TYPE_LOCAL_APIC, 8, 0);
    uint32_t currentCpu = 0;
    smp_proc_t *lastProc = NULL;
    while (acpiCpu != NULL && currentCpu < procCount) {
        if (acpiCpu->LapicFlags & ACPI_MADT_ENABLED) {
            // Create processor object.
            smp_proc_t *proc = (smp_proc_t*)kheap_alloc(sizeof(smp_proc_t));
            memset(proc, 0, sizeof(smp_proc_t));

            // Populate processor object with next available index.
            proc->ApicId = acpiCpu->Id;
            proc->Index = currentCpu;

            // Add to processor list.
            if (lastProc != NULL)
                lastProc->Next = proc;
            else
                processors = lastProc = proc;
            lastProc = proc;
            currentCpu++;
        }

        // Move to next CPU in ACPI.
        acpiCpu = (ACPI_MADT_LOCAL_APIC*)acpi_search_madt(ACPI_MADT_TYPE_LOCAL_APIC, 8, ((uintptr_t)acpiCpu) + 1);
    }

    kprintf("SMP: Waiting two seconds for verification...\n");
    sleep(2000);

    // Initialize boot code and stacks for APs.
    kprintf("SMP: Initializing %u processors...\n", procCount);
    smp_setup_apboot();
    smp_setup_stacks();

    // Initialize each processor.
    smp_proc_t *currentProc = processors;
    while (currentProc != NULL) {
        // No need to initialize the BSP (current processor).
        if (currentProc->ApicId == lapic_id()) {
            currentProc->Started = true;
            currentProc = currentProc->Next;
            continue;
        }

        // Send INIT command to processor (AP).
        kprintf("SMP: Sending INIT to LAPIC %u on processor %u\n", currentProc->ApicId, currentProc->Index);
        lapic_send_init(currentProc->ApicId);

        // Wait 10ms.
        sleep(10);

        // Send STARTUP command to processor (AP).
        kprintf("SMP: Sending STARTUP to LAPIC %u on processor %u\n", currentProc->ApicId, currentProc->Index);
        lapic_send_startup(currentProc->ApicId, SMP_AP_BOOTSTRAP_ADDRESS / PAGE_SIZE_4K);

        // Wait for processor to come up and move to next one.
        while (!currentProc->Started);
        currentProc = currentProc->Next;
    }

    // Destroy AP boot code.
    smp_destroy_apboot();
    kprintf("SMP: Initialized!\n");
}
