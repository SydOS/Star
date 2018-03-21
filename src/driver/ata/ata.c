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

bool ata_check_status(uint16_t portCommand, bool master) {
    // Get value of selected drive and ensure it is correct.
    if (master && (inb(ATA_REG_DRIVE_SELECT(portCommand)) & 0x10))
        return false;
    else if (!master && !(inb(ATA_REG_DRIVE_SELECT(portCommand)) & 0x10))
        return false;

    // Ensure status flags are good.
    uint8_t status = inb(ATA_REG_STATUS(portCommand));
    if (status & ATA_STATUS_BUSY || status & ATA_STATUS_DRIVE_FAULT || status & ATA_STATUS_ERROR || !(status & ATA_STATUS_READY))
        return false;
    return true;
}

bool ata_data_ready(uint16_t portCommand) {
    return inb(ATA_REG_STATUS(portCommand)) & ATA_STATUS_DATA_REQUEST;
}

static void ata_print_device_info(ata_identify_result_t info) {
    kprintf("ATA:    Model: %s\n", info.model);
    kprintf("ATA:    Firmware: %s\n", info.firmwareRevision);
    kprintf("ATA:    Serial: %s\n", info.serial);
    kprintf("ATA:    ATA versions:");
    if (info.versionMajor & ATA_VERSION_ATA1)
        kprintf(" ATA-1");
    if (info.versionMajor & ATA_VERSION_ATA2)
        kprintf(" ATA-2");
    if (info.versionMajor & ATA_VERSION_ATA3)
        kprintf(" ATA-3");
    if (info.versionMajor & ATA_VERSION_ATA4)
        kprintf(" ATA-4");
    if (info.versionMajor & ATA_VERSION_ATA5)
        kprintf(" ATA-5");
    if (info.versionMajor & ATA_VERSION_ATA6)
        kprintf(" ATA-6");
    if (info.versionMajor & ATA_VERSION_ATA7)
        kprintf(" ATA-7");
    if (info.versionMajor & ATA_VERSION_ATA8)
        kprintf(" ATA-8");
    kprintf("\n");

    // Is the device SATA?
    if (!(info.serialAtaFlags76 == 0x0000 || info.serialAtaFlags76 == 0xFFFF)) {
        kprintf("ATA:    Type: SATA\n");
        kprintf("ATA:        Gen1 support: %s\n", info.serialAtaFlags76 & ATA_SATA76_GEN1_SUPPORTED ? "yes" : "no");
        kprintf("ATA:        Gen2 support: %s\n", info.serialAtaFlags76 & ATA_SATA76_GEN2_SUPPORTED ? "yes" : "no");
        kprintf("ATA:        NCQ support: %s\n", info.serialAtaFlags76 & ATA_SATA76_NCQ_SUPPORTED ? "yes" : "no");
        kprintf("ATA:        Host-initiated power events: %s\n", info.serialAtaFlags76 & ATA_SATA76_POWER_SUPPORTED ? "yes" : "no");
        kprintf("ATA:        Phy events: %s\n", info.serialAtaFlags76 & ATA_SATA76_PHY_EVENTS_SUPPORTED ? "yes" : "no");
    }
    else {
        kprintf("ATA:    Type: PATA\n");
    }
}

static void ata_print_device_packet_info(ata_identify_packet_result_t info) {
    kprintf("ATA:    Model: %s\n", info.model);
    kprintf("ATA:    Firmware: %s\n", info.firmwareRevision);
    kprintf("ATA:    Serial: %s\n", info.serial);
    kprintf("ATA:    ATA versions:");
    if (info.versionMajor & ATA_VERSION_ATA1)
        kprintf(" ATAPI-1");
    if (info.versionMajor & ATA_VERSION_ATA2)
        kprintf(" ATAPI-2");
    if (info.versionMajor & ATA_VERSION_ATA3)
        kprintf(" ATAPI-3");
    if (info.versionMajor & ATA_VERSION_ATA4)
        kprintf(" ATAPI-4");
    if (info.versionMajor & ATA_VERSION_ATA5)
        kprintf(" ATAPI-5");
    if (info.versionMajor & ATA_VERSION_ATA6)
        kprintf(" ATAPI-6");
    if (info.versionMajor & ATA_VERSION_ATA7)
        kprintf(" ATAPI-7");
    if (info.versionMajor & ATA_VERSION_ATA8)
        kprintf(" ATAPI-8");
    kprintf("\n");

    // Is the device SATA?
    if (!(info.serialAtaFlags76 == 0x0000 || info.serialAtaFlags76 == 0xFFFF)) {
        kprintf("ATA:    Type: SATA\n");
        kprintf("ATA:    Gen1 support: %s\n", info.serialAtaFlags76 & ATA_SATA76_GEN1_SUPPORTED ? "yes" : "no");
        kprintf("ATA:    Gen2 support: %s\n", info.serialAtaFlags76 & ATA_SATA76_GEN2_SUPPORTED ? "yes" : "no");
        kprintf("ATA:    NCQ support: %s\n", info.serialAtaFlags76 & ATA_SATA76_NCQ_SUPPORTED ? "yes" : "no");
        kprintf("ATA:    Host-initiated power events: %s\n", info.serialAtaFlags76 & ATA_SATA76_POWER_SUPPORTED ? "yes" : "no");
        kprintf("ATA:    Phy events: %s\n", info.serialAtaFlags76 & ATA_SATA76_PHY_EVENTS_SUPPORTED ? "yes" : "no");
    }
    else {
        kprintf("ATA:    Type: PATA\n");
    }
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
    // Identify primary master.
    ata_identify_result_t priMaster;
    if (ata_identify(ATA_PRI_COMMAND_PORT, ATA_PRI_CONTROL_PORT, true, &priMaster)) {
        kprintf("ATA: Found master device on primary channel!\n");
        ata_print_device_info(priMaster);
    }
    else {
        kprintf("ATA: No master device found on primary channel.\n");
    }

    // Identify primary slave.
    ata_identify_result_t priSlave;
    if (ata_identify(ATA_PRI_COMMAND_PORT, ATA_PRI_CONTROL_PORT, false, &priSlave)) {
        kprintf("ATA: Found slave device on primary channel!\n");
        ata_print_device_info(priSlave);
    }
    else {
        kprintf("ATA: No slave device found on primary channel.\n");
    }

    // Identify secondary master.
    ata_identify_result_t secMaster;
    if (ata_identify(ATA_SEC_COMMAND_PORT, ATA_SEC_CONTROL_PORT, true, &secMaster)) {
        kprintf("ATA: Found master device on secondary channel!\n");
        ata_print_device_info(secMaster);
    }
    else {
        // Identify secondary master.
        ata_identify_packet_result_t secMasterPacket;
        if (ata_identify_packet(ATA_SEC_COMMAND_PORT, ATA_SEC_CONTROL_PORT, true, &secMasterPacket)) {
            kprintf("ATA: Found ATAPI master device on secondary channel!\n");
            ata_print_device_packet_info(secMasterPacket);
        }
        else {
            kprintf("ATA: No master device found on secondary channel.\n");
        }
    }

    // Identify secondary slave.
    ata_identify_result_t secSlave;
    if (ata_identify(ATA_SEC_COMMAND_PORT, ATA_SEC_CONTROL_PORT, false, &secSlave)) {
        kprintf("ATA: Found slave device on secondary channel!\n");
        ata_print_device_info(secSlave);
    }
    else {
        kprintf("ATA: No slave device found on secondary channel.\n");
    }
}
