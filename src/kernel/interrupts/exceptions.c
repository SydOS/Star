#include <main.h>
#include <kprint.h>
#include <kernel/interrupts/exceptions.h>

#include <kernel/interrupts/idt.h>
#include <kernel/interrupts/lapic.h>

// Functions defined by Intel for service extensions.
extern void _exception0();
extern void _exception1();
extern void _exception2();
extern void _exception3();
extern void _exception4();
extern void _exception5();
extern void _exception6();
extern void _exception7();
extern void _exception8();
extern void _exception9();
extern void _exception10();
extern void _exception11();
extern void _exception12();
extern void _exception13();
extern void _exception14();
extern void _exception16();
extern void _exception17();
extern void _exception18();
extern void _exception19();
extern void _exception20();

// Array of exception handler pointers.
static exception_handler exceptionHandlers[EXCEPTION_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

// Exception messages array.
static char *exception_messages[] = {
    "Divide By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Installs an exception handler.
void exceptions_install_handler(uint8_t exception, exception_handler handler) {
    // Add handler for exception to array.
    exceptionHandlers[exception] = handler;
    kprintf("EXCEPTIONS: Handler for exception %u installed!\n", exception);
}

// Removes an exception handler.
void exceptions_remove_handler(uint8_t exception) {
    // Null out handler for exception in array.
    exceptionHandlers[exception] = 0;
    kprintf("EXCEPTIONS: Handler for exception %u removed!\n", exception);
}

// Handler for exceptions.
void exceptions_handler(ExceptionRegisters_t *regs) {
    // Get handler.
    exception_handler handler;
    handler = exceptionHandlers[regs->intNum];

    // Invoke any handler registered. Otherwise panic with default message.
    if (handler)
        handler(regs);
    else
        panic("CPU%d: Exception: %s (error code: %p)\n", lapic_id(), exception_messages[regs->intNum], regs->errorCode);
}

void exceptions_init(idt_entry_t *idt) {
    // Add exceptions to the IDT.
    kprintf("EXCEPTIONS: Initializing exceptions...\n");
    idt_open_interrupt_gate(idt, 0, (uintptr_t)_exception0);
    idt_open_interrupt_gate(idt, 1, (uintptr_t)_exception1);
    idt_open_interrupt_gate(idt, 2, (uintptr_t)_exception2);
    idt_open_interrupt_gate(idt, 3, (uintptr_t)_exception3);
    idt_open_interrupt_gate(idt, 4, (uintptr_t)_exception4);
    idt_open_interrupt_gate(idt, 5, (uintptr_t)_exception5);
    idt_open_interrupt_gate(idt, 6, (uintptr_t)_exception6);
    idt_open_interrupt_gate(idt, 7, (uintptr_t)_exception7);
    idt_open_interrupt_gate(idt, 8, (uintptr_t)_exception8);
    idt_open_interrupt_gate(idt, 9, (uintptr_t)_exception9);
    idt_open_interrupt_gate(idt, 10, (uintptr_t)_exception10);
    idt_open_interrupt_gate(idt, 11, (uintptr_t)_exception11);
    idt_open_interrupt_gate(idt, 12, (uintptr_t)_exception12);
    idt_open_interrupt_gate(idt, 13, (uintptr_t)_exception13);
    idt_open_interrupt_gate(idt, 14, (uintptr_t)_exception14);
    idt_open_interrupt_gate(idt, 16, (uintptr_t)_exception16);
    idt_open_interrupt_gate(idt, 17, (uintptr_t)_exception17);
    idt_open_interrupt_gate(idt, 18, (uintptr_t)_exception18);
    idt_open_interrupt_gate(idt, 19, (uintptr_t)_exception19);
    idt_open_interrupt_gate(idt, 20, (uintptr_t)_exception20);
    kprintf("EXCEPTIONS: Initialized!\n");
}