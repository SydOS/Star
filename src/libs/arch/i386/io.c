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
void cpu_msr_read(uint32_t msr, uint32_t *low, uint32_t *high) {
    asm volatile ("rdmsr" : "=a"(*low), "=d"(high) : "c"(msr));
}
