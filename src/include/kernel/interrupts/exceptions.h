#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <main.h>

#define EXCEPTION_COUNT     32

// Exceptions.
// https://wiki.osdev.org/Exceptions
enum {
    EXCEPTION_DIVIDE_BY_ZERO            = 0,
    EXCEPTION_DEBUG                     = 1,
    EXCEPTION_NON_MASKABLE_INTERRUPT    = 2,
    EXCEPTION_BREAKPOINT                = 3,
    EXCEPTION_OVERFLOW                  = 4,
    EXCEPTION_BOUND_RANGE_EXCEEDED      = 5,
    EXCEPTION_INVALID_OPCODE            = 6,
    EXCEPTION_DEVICE_NOT_AVAILABLE      = 7,
    EXCEPTION_DOUBLE_FAULT              = 8,
    EXCEPTION_COPROCESSOR_SEG_OVERRUN   = 9,
    EXCEPTION_INVALID_TSS               = 10,
    EXCEPTION_SEGMENT_NOT_PRESENT       = 11,
    EXCEPTION_STACK_SEGMENT_FAULT       = 12,
    EXCEPTION_GENERAL_PROTECTION_FAULT  = 13,
    EXCEPTION_PAGE_FAULT                = 14,
    EXCEPTION_X87_FLOAT_POINT           = 16,
    EXCEPTION_ALIGNMENT_CHECK           = 17,
    EXCEPTION_MACHINE_CHECK             = 18,
    EXCEPTION_SIMD_FLOAT_POINT          = 19,
    EXCEPTION_VIRTUALIZATION            = 20,
    EXCEPTION_SECURITY                  = 30
};

// Defines registers for exception callbacks.
typedef struct {
    // Extra segments and data segment.
    uintptr_t gs, fs, es, ds;

#ifdef X86_64
    // Extra x64 registers.
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
#endif
    // Destination index, source index, base pointer.
    uintptr_t di, si, bp;

#ifndef X86_64
    // Stack pointer (only in 32-bit).
    uintptr_t sp;
#endif

    // Base, data, counter, and accumulator registers.
    uintptr_t bx, dx, cx, ax;

    // Interrupt number and error code.
    uintptr_t intNum, errorCode;

    // Instruction pointer, code segment, and flags.
    uintptr_t ip, cs, flags;
    
#ifdef X86_64
    // Stack pointer (only in 64-bit).
    uintptr_t sp;
#else
    // Stack pointer used when changing rings (only in 32-bit).
    uintptr_t usersp;
#endif
    
    // Stack segment.
    uintptr_t ss;   
} __attribute__((packed)) ExceptionRegisters_t;

// Exception handler type.
typedef void (*exception_handler)(ExceptionRegisters_t *regs);

extern void exceptions_install_handler(uint8_t exception, exception_handler handler);
extern void exceptions_remove_handler(uint8_t exception);
extern void exceptions_init(void);

#endif
