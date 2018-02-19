#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/pic.h>

#define PIC1            0x20
#define PIC2            0xA0
#define PIC1_COMMAND    PIC1
#define PIC1_DATA       (PIC1+1)
#define PIC2_COMMAND    PIC2
#define PIC2_DATA       (PIC2+1)

#define ICW1_ICW4	    0x01 // ICW4 (not) needed.
#define ICW1_INIT	    0x10 // Initialization - required!
#define ICW4_8086       0x01 // 8086/88 (MCS-80/85) mode.

#define PIC_COMMAND_EOI 0x20

// Remaps the PIC.
static void pic_remap(uint8_t offset1, uint8_t offset2)
{
    outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);

    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);

    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
}

// Sends an EOI to the PICs.
void pic_eoi(uint32_t interrupt)
{
    // If the IDT entry was greater than 40 (IRQ 8 to 15),
    // we need to send an EOI to the slave PCI too.
    if (interrupt >= 40)
        outb(PIC2_COMMAND, PIC_COMMAND_EOI);

    // Send EOI to master PIC.
    outb(PIC1_COMMAND, PIC_COMMAND_EOI);
}

// Enables the PIC.
void pic_enable()
{
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Enable PICs (unmask interrupts).
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);
    kprintf("PIC enabled!\n");
}

// Disables the PIC.
void pic_disable()
{
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Disable PICs (mask all interrupts).
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    kprintf("PIC disabled!\n");
}

void pic_init()
{
    // Enable the PIC.
    pic_enable();
}
