#include <main.h>
#include <kprint.h>
#include <string.h>
#include <io.h>

#include <kernel/multitasking/syscalls.h>
#include <kernel/gdt.h>
#include <kernel/interrupts/idt.h>
#include <kernel/pit.h>
#include <kernel/cpuid.h>
#include <kernel/memory/kheap.h>


uint8_t *SyscallStack;

#ifdef X86_64
extern uintptr_t _syscalls_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);
extern void _syscalls_syscall_handler(void);
#else
// SYSENTER stuff.
extern uintptr_t _syscalls_sysenter(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);
extern void _syscalls_sysenter_handler(void);
#endif

// Interrupt stuff.
static bool syscallInterruptOnly = true;
extern uintptr_t _syscalls_interrupt(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);
extern void _syscalls_interrupt_handler(void);

uintptr_t syscalls_syscall(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index) {
    // If we can only do interrupts, do it. Otherwise do a fast syscall.
    if (syscallInterruptOnly)
        return _syscalls_interrupt(arg0, arg1, arg2, arg3, arg4, arg5, index);
    else
#ifdef X86_64
        return _syscalls_syscall(arg0, arg1, arg2, arg3, arg4, arg5, index);
#else
        return _syscalls_sysenter(arg0, arg1, arg2, arg3, arg4, arg5, index);
#endif
}

void syscalls_kprintf(const char *format, ...) {
    // Get args.
    va_list args;
	va_start(args, format);

	// Invoke kprintf_va via system call.
    syscalls_syscall(format, args, 0, 0, 0, 0, 0xAB);
}

static uintptr_t syscalls_uptime_handler(uintptr_t ptrAddr) {
    uint64_t *uptimePtr = (uint64_t)ptrAddr;
    if (uptimePtr != NULL) {
        *uptimePtr = pit_ticks() / 1000;
        return 0;
    }
    return -1;
}

uintptr_t syscalls_handler(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index) {
    if (index == 0xAB) {
       kprintf_va(arg0, arg1);
       return 0xFE;
    }
    
    switch (index) {
        case SYSCALL_UPTIME:
            return syscalls_uptime_handler(arg0);

        default:
            return -1;
    }

    return 0xFE;
}

void syscalls_init(void) {
#ifdef X86_64
    // Detect and enable SYSCALL.
    uint32_t result, unused;
    if (cpuid_query(CPUID_INTELFEATURES, &unused, &unused, &unused, &result) && (result & CPUID_FEAT_EDX_SYSCALL)) {
        kprintf("SYSCALLS: SYSCALL instruction supported!\n");
        SyscallStack = kheap_alloc(4096);
        memset(SyscallStack, 0, 4096);

        cpu_msr_write(SYSCALL_MSR_STAR, (((uint64_t)(GDT_KERNEL_DATA_OFFSET | GDT_SELECTOR_RPL_RING3)) << 48) | ((uint64_t)GDT_KERNEL_CODE_OFFSET << 32));
        cpu_msr_write(SYSCALL_MSR_LSTAR, (uintptr_t)_syscalls_syscall_handler);

        // Enable SYSCALL instructions.
        cpu_msr_write(SYSCALL_MSR_EFER, cpu_msr_read(SYSCALL_MSR_EFER) | 0x1);
        syscallInterruptOnly = false;
        kprintf("SYSCALLS: SYSCALL instruction enabled!\n");
    }
#else
    // Determine if SYSENTER is supported (only on PII or higher).
    uint32_t resultEax, resultEdx, unused;
    if (cpuid_query(CPUID_GETFEATURES, &resultEax, &unused, &unused, &resultEdx) && (!(CPUID_FAMILY(resultEax) == 6 &&
        CPUID_MODEL(resultEax) < 3 && CPUID_STEPPING(resultEax) < 3)) && (resultEdx & CPUID_FEAT_EDX_SEP)) {
        // We can use the SYSENTER instruction for better performance.
        kprintf("SYSCALLS: SYSENTER instruction supported!\n");
        SyscallStack = kheap_alloc(4096);
        memset(SyscallStack, 0, 4096);

        // Set ring 0 CS, ESP, and EIP.
        cpu_msr_write(SYSCALL_MSR_SYSENTER_CS, GDT_KERNEL_CODE_OFFSET);
        cpu_msr_write(SYSCALL_MSR_SYSENTER_ESP, (uintptr_t)SyscallStack + 4096);
        cpu_msr_write(SYSCALL_MSR_SYSENTER_EIP, (uint32_t)_syscalls_sysenter_handler);
        syscallInterruptOnly = false;
        kprintf("SYSCALLS: SYSENTER instruction enabled!\n");
    }
#endif

    // Add interrupt option for syscalls.
    idt_set_gate(SYSCALL_INTERRUPT, (uintptr_t)_syscalls_interrupt_handler, GDT_KERNEL_CODE_OFFSET, IDT_GATE_INTERRUPT_32, GDT_PRIVILEGE_USER, true);
    kprintf("SYSCALLS: Added handler for interrupt 0x%X at 0x%p\n", SYSCALL_INTERRUPT, _syscalls_interrupt_handler);
    kprintf("SYSCALLS: Initialized!\n");
}
