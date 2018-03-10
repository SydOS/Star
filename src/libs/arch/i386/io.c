#include <main.h>
#include <io.h>

// Outputs a byte to the specified port.
void outb(uint16_t port, uint8_t data)
{
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Gets a byte from the specified port.
uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

void io_wait() {
    asm volatile ("jmp 1f\n\t"
                  "1:jmp 2f\n\t"
                  "2:");
}

// Read MSR.
uint64_t cpu_msr_read(uint32_t msr) {
    uint32_t low, high;
    asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

// Write MSR.
void cpu_msr_write(uint32_t msr, uint64_t value) {
    asm volatile ("wrmsr" : : "a"((uint32_t)(value & 0xFFFFFFFF)), "d"((uint32_t)(value >> 32)), "c"(msr));
}
