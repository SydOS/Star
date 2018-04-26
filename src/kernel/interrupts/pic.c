#include <main.h>
#include <kprint.h>
#include <io.h>
#include <kernel/interrupts/pic.h>

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

// Enables the PIC.
void pic_enable(void) {
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Enable PICs (unmask interrupts).
    outb(PIC1_DATA, 0x0);
    io_wait();
    outb(PIC2_DATA, 0x0);
    io_wait();
    kprintf("PIC: Enabled!\n");
}

// Disables the PIC.
void pic_disable(void) {
    // Re-map PIC.
    pic_remap(0x20, 0x28);

    // Disable PICs (mask all interrupts).
    outb(PIC1_DATA, PIC_CMD_DISABLE);
    io_wait();
    outb(PIC2_DATA, PIC_CMD_DISABLE);
    io_wait();

    // Ensure any pending interrupts are clear.
    pic_eoi(40);
    kprintf("PIC: Disabled!\n");
}

// Sends an EOI to the PICs.
void pic_eoi(uint32_t irq) {
    // If the IRQ was greater than 7 (IRQ 8 to 15),
    // we need to send an EOI to the slave PCI too.
    if (irq >= 8)
        outb(PIC2_CMD, PIC_CMD_EOI);

    // Send EOI to master PIC.
    outb(PIC1_CMD, PIC_CMD_EOI);
    io_wait();
}

static uint16_t pic_get_irq_reg(uint8_t reg) {
    // Send command to PICs to get register.
    outb(PIC1_CMD, reg);
    io_wait();
    outb(PIC2_CMD, reg);
    io_wait();

    // Get value of specified register from PICs.
    return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

uint16_t pic_get_irr(void) {
    // Get value of IRR register.
    return pic_get_irq_reg(PIC_CMD_IRR);
}

uint16_t pic_get_isr(void) {
    // Get value of ISR register.
    return pic_get_irq_reg(PIC_CMD_ISR);
}

uint8_t pic_get_irq(void) {
    // Get value of ISR register and mask out bit 2, as that is set when
    // the slave PIC has an interrupt waiting.
    uint16_t isr = pic_get_irq_reg(PIC_CMD_ISR);
    isr &= ~(1 << 2);

    // Return the number of 0s after the set bit, which indicate the IRQ number.
    return __builtin_ctz(isr);
}

void pic_init(void) {
    // Enable the PIC.
    kprintf("PIC: Initializing...\n");
    pic_enable();
    kprintf("PIC: Initialized!\n");
}
