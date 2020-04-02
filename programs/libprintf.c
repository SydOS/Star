#include <stdarg.h>
#include <stdint.h>

extern uintptr_t _syscalls_interrupt(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t index);

void syscalls_kprintf(const char *format, ...) {
    // Get args.
    va_list args;
	va_start(args, format);

    _syscalls_interrupt(format, args, 0, 0, 0, 0, 0xAB);
}