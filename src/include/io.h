#ifndef IO_H
#define IO_H

/**
 * Outputs a byte to the specified port
 * @param port uint16_t for the port address
 * @param val  uint8_t for the byte to write
 */
static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

/**
 * Gets a byte from the specified port
 * @param  port uint16_t for the port address
 * @return      uint8_t containing the byte read
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

/**
 * Waits for an I/O operation to complete
 */
static inline void io_wait(void)
{
    /* TODO: This is probably fragile. */
    asm volatile ( "jmp 1f\n\t"
                   "1:jmp 2f\n\t"
                   "2:" );
}

#endif