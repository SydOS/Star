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

extern void smp_init();

#endif
