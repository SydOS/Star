#ifndef SMP_H
#define SMP_H

#include <main.h>

#define SMP_PAGING_ADDRESS          0x500
#define SMP_PAGING_PAE_ADDRESS      0x510
#define SMP_GDT_ADDRESS             0x600
#define SMP_AP_BOOTSTRAP_ADDRESS    0x7000

extern void smp_init();

#endif
