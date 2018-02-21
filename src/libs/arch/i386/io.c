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

// -----------------------------------------------------------------------------

// Outputs 2 bytes to the specified port.
void outw(uint16_t port, uint16_t data)
{
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

// Gets 2 bytes from the specified port.
uint16_t inw(uint16_t port)
{
    uint8_t data;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// -----------------------------------------------------------------------------

// Outputs 4 bytes to the specified port.
void outl(uint16_t port, uint32_t data)
{
    asm volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

// Gets 4 bytes from the specified port.
uint32_t inl(uint16_t port)
{
    uint8_t data;
    asm volatile("inl %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}