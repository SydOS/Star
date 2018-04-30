#ifndef GDT_H
#define GDT_H

#include <main.h>

#ifdef X86_64
#define GDT32_ENTRIES 5
#define GDT64_ENTRIES 7
#else
#define GDT32_ENTRIES 6
#endif

#define GDT_PRIVILEGE_KERNEL    0x0
#define GDT_PRIVILEGE_USER      0x3

// This structure contains the value of one GDT entry.
// We use the attribute 'packed' to tell GCC not to change
// any of the alignment in the structure.
typedef struct {
    // Low 16 bits of limit.
    uint16_t LimitLow : 16;

    // Low 24 bits of base address.
    uint32_t BaseLow : 24;

    // Access bits.
    bool Accessed : 1;
    bool Writeable : 1;
    bool DirectionConforming : 1;
    bool Executable : 1;
    bool DescriptorBit : 1;
    uint8_t Privilege : 2;
    bool Present : 1;

    // High 4 bits of limit.
    uint8_t LimitHigh : 4;

    // Flags bits.
    uint8_t ReservedZero : 1;
    bool Is64Bits : 1;
    bool Is32Bits : 1;
    bool IsLimit4K : 1;

    // High 8 bits of base address.
    uint8_t BaseHigh : 8;
} __attribute__((packed)) gdt_entry_t;

// This struct describes a GDT pointer. It points to the start of
// our array of GDT entries, and is in the format required by the
// lgdt instruction.
typedef struct {
    // The upper 16 bits of all selector limits.
    uint16_t Limit;

    // The address of the first GDT entry.               
    uintptr_t Base;
} __attribute__((packed)) gdt_ptr_t;

#define GDT_NULL_INDEX          0
#define GDT_KERNEL_CODE_INDEX   1
#define GDT_KERNEL_DATA_INDEX   2
#define GDT_USER_DATA_INDEX     3
#define GDT_USER_CODE_INDEX     4
#define GDT_TSS_INDEX           5

// GDT offsets. SYSCALL requires user code to be after user data for some reason.
#define GDT_NULL_OFFSET         (uint8_t)(GDT_NULL_INDEX * sizeof(gdt_entry_t))
#define GDT_KERNEL_CODE_OFFSET  (uint8_t)(GDT_KERNEL_CODE_INDEX * sizeof(gdt_entry_t))
#define GDT_KERNEL_DATA_OFFSET  (uint8_t)(GDT_KERNEL_DATA_INDEX * sizeof(gdt_entry_t))
#define GDT_USER_DATA_OFFSET    (uint8_t)(GDT_USER_DATA_INDEX * sizeof(gdt_entry_t))
#define GDT_USER_CODE_OFFSET    (uint8_t)(GDT_USER_CODE_INDEX * sizeof(gdt_entry_t))
#define GDT_TSS_OFFSET          (uint8_t)(GDT_TSS_INDEX * sizeof(gdt_entry_t))

#define GDT_SELECTOR_RPL_RING3  0x3

extern void gdt_tss_set_kernel_stack(uintptr_t stack);
extern void gdt_load(void);
extern void gdt_tss_load(void);
extern void gdt_init(void);

#endif
