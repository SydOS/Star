#ifndef GDT_H
#define GDT_H

#include <main.h>

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
struct gdt_entry {
    uint16_t limitLow;          // The lower 16 bits of the limit.
    uint16_t baseLow;           // The lower 16 bits of the base.
    uint8_t  baseMiddle;        // The next 8 bits of the base.
    uint8_t  access;            // Access flags, determine what ring this segment can be used in.
    uint8_t  granularity;       // Limit and flags
    uint8_t  baseHigh;          // The last 8 bits of the base.
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;

// This struct describes a GDT pointer. It points to the start of
// our array of GDT entries, and is in the format required by the
// lgdt instruction.
struct gdt_ptr {
    uint16_t limit;               // The upper 16 bits of all selector limits.
    uintptr_t base;               // The address of the first gdt_entry_t struct.
} __attribute__((packed));
typedef struct gdt_ptr gdt_ptr_t;

#define GDT32_ENTRIES 5
#define GDT64_ENTRIES 5

extern void gdt_load();
extern void gdt_init();

#endif
