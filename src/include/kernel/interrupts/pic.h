#ifndef PIC_H
#define PICH

#include <main.h>

#define PIC1_CMD        0x20
#define PIC2_CMD        0xA0
#define PIC1_DATA       0x21
#define PIC2_DATA       0xA1

#define PIC_CMD_EOI     0x20
#define PIC_CMD_INIT    0x11
#define PIC_CMD_DISABLE 0xFF
#define PIC_CMD_8086    0x01

extern void pic_eoi(uint32_t irq);
extern void pic_init();
extern void pic_disable();

#endif
