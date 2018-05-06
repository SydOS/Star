#ifndef TSS_H
#define TSS_H

#include <main.h>

#ifdef X86_64
// 64-bit TSS.
typedef struct {
    uint32_t Reserved1;

    // Ring stack pointers.
    uint64_t RSP0;
    uint64_t RSP1;
    uint64_t RSP2;
    uint64_t Reserved2;

    // Interrupt stack tables.
    uint64_t IST1;
    uint64_t IST2;
    uint64_t IST3;
    uint64_t IST4;
    uint64_t IST5;
    uint64_t IST6;
    uint64_t IST7;

    uint64_t Reserved3;
    uint16_t Reserved4;
    uint16_t IoPermissionsBaseOffset;
} __attribute__((packed)) tss_t;

#else
// 32-bit TSS.
typedef struct {
    // Link to next TSS.
    uint32_t Link;

    // Ring stacks.
    uint32_t ESP0;
    uint32_t SS0;
    uint32_t ESP1;
    uint32_t SS1;
    uint32_t ESP2;
    uint32_t SS2;

    // Registers.
    uint32_t CR3;
    uint32_t EIP;
    uint32_t EFLAGS;
    uint32_t EAX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t EBX;
    uint32_t ESP;
    uint32_t EBP;
    uint32_t ESI;
    uint32_t EDI;

    // Segments.
    uint32_t ES;
    uint32_t CS;
    uint32_t SS;
    uint32_t DS;
    uint32_t FS;
    uint32_t GS;
    
    uint32_t LdtSelector;
    uint16_t Trap;
    uint16_t IoPermissionsBaseOffset;
} __attribute__((packed)) tss_t;
#endif

#endif
