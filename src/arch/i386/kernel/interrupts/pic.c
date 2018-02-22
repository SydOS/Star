#include <main.h>
#include <kprint.h>
#include <io.h>
#include <arch/i386/kernel/pic.h>

// Remaps the PIC.
static void pic_remap(uint8_t offset1, uint8_t offset2) {
    // Get current masks.
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // Begin initialization of PICs.
    outb(PIC1_CMD, PIC_CMD_INIT);
    io_wait();
    outb(PIC2_CMD, PIC_CMD_INIT);
    io_wait();

    // Set new offsets.
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // Enable cascading.
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    
    // Place PICs in 8086 mode.
    outb(PIC1_DATA, PIC_CMD_8086);
    io_wait();
    outb(PIC2_DATA, PIC_CMD_8086);
    io_wait();

    // Set original masks.
    outb(PIC1_DATA, mask1);
    io_wait();
    outb(PIC2_DATA, mask2);
    io_wait();
}

// Sends an EOI to the PICs.
void pic_eoi(uint32_t interrupt) {
    // If the IDT entry was greater than 40 (IRQ 8 to 15),
    // we need to send an EOI to the slave PCI too.
    if (interrupt >= 40)
        outb(PIC2_CMD, PIC_CMD_EOI);

    // Send EOI to master PIC.
    outb(PIC1_CMD, PIC_CMD_EOI);
    io_wait();
}

// Enables the PIC.
void pic_enable() {
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Enable PICs (unmask interrupts).
    outb(PIC1_DATA, 0x0);
    io_wait();
    outb(PIC2_DATA, 0x0);
    io_wait();
    kprintf("PIC enabled!\n");
}

// Disables the PIC.
void pic_disable() {
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Disable PICs (mask all interrupts).
    outb(PIC1_DATA, PIC_CMD_DISABLE);
    io_wait();
    outb(PIC2_DATA, PIC_CMD_DISABLE);
    io_wait();

    // Ensure any pending interrupts are clear.
    pic_eoi(40);
    kprintf("PIC disabled!\n");
}

void pic_init() {
    // Enable the PIC.
    pic_enable();
}
