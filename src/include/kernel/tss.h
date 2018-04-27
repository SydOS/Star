#ifndef TSS_H
#define TSS_H

#include <main.h>

typedef struct {
    uint32_t PreviousTaskLink;
    uint32_t Esp0;
    uint32_t Ss0;
    uint32_t Esp1;
    uint32_t Ss1;
    uint32_t Esp2;
    uint32_t Ss2;
    uint32_t Cr3;

    uint32_t Eip;
    uint32_t Eflags;
    uint32_t Eax;
    uint32_t Ecx;
    uint32_t Edx;
    uint32_t Ebx;
    uint32_t Esp;
    uint32_t Ebp;
    uint32_t Esi;
    uint32_t Edi;

    uint32_t Es;
    uint32_t Cs;
    uint32_t Ss;
    uint32_t Ds;
    uint32_t Fs;
    uint32_t Gs;

    uint32_t LdtSegmentSelector;
    uint16_t Trap;
    uint16_t BaseAddress;
} __attribute__((packed)) tss_entry_t;

#endif
