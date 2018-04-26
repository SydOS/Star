#ifndef PIC_H
#define PIC_H

#include <main.h>

// PIC master and slave ports.
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21
#define PIC2_CMD        0xA0
#define PIC2_DATA       0xA1

// PIC commands.
#define PIC_CMD_8086    0x01
#define PIC_CMD_IRR     0x0A
#define PIC_CMD_ISR     0x0B
#define PIC_CMD_INIT    0x11
#define PIC_CMD_EOI     0x20
#define PIC_CMD_DISABLE 0xFF

extern void pic_enable(void);
extern void pic_disable(void);

extern void pic_eoi(uint32_t irq);
extern uint16_t pic_get_irr(void);
extern uint16_t pic_get_isr(void);
extern uint8_t pic_get_irq(void);
extern void pic_init(void);

#endif
