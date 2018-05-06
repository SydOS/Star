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
