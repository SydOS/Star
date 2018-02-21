#ifndef IO_H
#define IO_H

extern void outb(uint16_t port, uint8_t data);
extern uint8_t inb(uint16_t);

extern void outw(uint16_t port, uint16_t data);
extern uint16_t inw(uint16_t);

extern void outl(uint16_t port, uint32_t data);
extern uint32_t inl(uint16_t);

#endif
