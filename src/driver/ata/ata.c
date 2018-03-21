#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/ata/ata.h>
#include <driver/ata/ata_commands.h>
#include <kernel/interrupts/interrupts.h>

bool irqTriggeredPri = false;
bool irqTriggeredSec = false;

static void ata_callback_pri() {
    irqTriggeredPri = true;
    kprintf("ATA: IRQ14 raised!\n");
}

static void ata_callback_sec() {
    irqTriggeredSec = true;
    kprintf("ATA: IRQ15 raised!\n");
}

uint16_t ata_read_data_word(uint16_t port) {
    return inw(ATA_REG_DATA(port));
}

void ata_soft_reset(uint16_t portControl) {
    // Cycle reset bit.
    outb(ATA_REG_DEVICE_CONTROL(portControl), ATA_DEVICE_CONTROL_RESET);
    io_wait();
    outb(ATA_REG_DEVICE_CONTROL(portControl), 0x00);
    io_wait();
    sleep(400);
}

void ata_select_device(uint16_t portCommand, uint16_t portControl, bool lba, bool master) {
    uint8_t value = master ? 0xA0 : 0xB0;
    if (lba)
        value |= 0x40;
    outb(ATA_REG_DRIVE_SELECT(portCommand), value);
    io_wait();
    inb(ATA_REG_ALT_STATUS(portControl));
    inb(ATA_REG_ALT_STATUS(portControl));
    inb(ATA_REG_ALT_STATUS(portControl));
    inb(ATA_REG_ALT_STATUS(portControl));

     // Get signature.
    inb(ATA_REG_SECTOR_COUNT(portCommand));
    inb(ATA_REG_SECTOR_NUMBER(portCommand));
    inb(ATA_REG_CYLINDER_LOW(portCommand));
    inb(ATA_REG_CYLINDER_HIGH(portCommand));
    inb(ATA_REG_DRIVE_SELECT(portCommand));

    // Zero out registers.
    outb(ATA_REG_SECTOR_COUNT(portCommand), 0x00);
    outb(ATA_REG_SECTOR_NUMBER(portCommand), 0x00);
    outb(ATA_REG_CYLINDER_LOW(portCommand), 0x00);
    outb(ATA_REG_CYLINDER_HIGH(portCommand), 0x00);
    io_wait();
}

void ata_init() {
    kprintf("ATA: Initializing...\n");

    // Register interrupt handlers.
    interrupts_irq_install_handler(IRQ_PRI_ATA, ata_callback_pri);
    interrupts_irq_install_handler(IRQ_SEC_ATA, ata_callback_sec);

    // Reset both channels.
    ata_soft_reset(ATA_PRI_CONTROL_PORT);
    ata_soft_reset(ATA_SEC_CONTROL_PORT);

    // Identify devices.
    // Identify master.
    ata_identify_result_t priMaster;
    if(ata_identify(ATA_PRI_COMMAND_PORT, ATA_PRI_CONTROL_PORT, true, &priMaster)) {
        kprintf("ATA: Found master device on primary channel!\n");
        kprintf("ATA:    Model: %s\n", priMaster.model);
        kprintf("ATA:    Firmware: %s\n", priMaster.firmwareRevision);
        kprintf("ATA:    Serial: %s\n", priMaster.serial);
    }        

}
