#ifndef IO_H
#define IO_H

extern void outb(uint16_t port, uint8_t data);
extern uint8_t inb(uint16_t);
extern void io_wait();
extern void cpu_msr_read(uint32_t msr, uint32_t *low, uint32_t *high);

#endif
