#include <main.h>
#include <kprint.h>
#include <string.h>
#include <io.h>

#include <kernel/multitasking/syscalls.h>
#include <kernel/gdt.h>
#include <kernel/pit.h>
#include <kernel/cpuid.h>
#include <kernel/memory/kheap.h>
extern void _syscalls_handler(void);

uint8_t *SyscallStack;
uintptr_t SyscallTemp;

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
        //va_list args = (va_list[])arg1;
        //uintptr_t *ar = &arg1;
       // kprintf_va((const char*)arg0, (va_list)ar);
       kprintf_va(arg0, arg1);
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
     // Detect and enable SYSCALL.
    uint32_t result, unused;
    if (cpuid_query(CPUID_INTELFEATURES, &unused, &unused, &unused, &result) && (result & CPUID_FEAT_EDX_SYSCALL)) {
        
        SyscallStack = kheap_alloc(4096);
        memset(SyscallStack, 0, 4096);

        cpu_msr_write(0xC0000081, (((uint64_t)(GDT_KERNEL_DATA_OFFSET | GDT_SELECTOR_RPL_RING3)) << 48) | ((uint64_t)GDT_KERNEL_CODE_OFFSET << 32));
uint64_t msr = cpu_msr_read(0xC0000081);
        cpu_msr_write(0xC0000082, (uint64_t)_syscalls_handler);

        // Enable SYSCALL instructions.
        msr = cpu_msr_read(0xC0000080);
        cpu_msr_write(0xC0000080, msr | 0x1);
        kprintf("SYSCALLS: SYSCALL instruction enabled!\n");
    }
}
