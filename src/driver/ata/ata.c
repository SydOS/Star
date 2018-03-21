#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/ata/ata.h>
#include <driver/ata/ata_commands.h>
#include <kernel/interrupts/interrupts.h>

static bool irqTriggeredPri = false;
static bool irqTriggeredSec = false;

static void ata_callback_pri() {
    irqTriggeredPri = true;
    kprintf("ATA: IRQ14 raised!\n");
}

static void ata_callback_sec() {
    irqTriggeredSec = true;
    kprintf("ATA: IRQ15 raised!\n");
}

bool ata_wait_for_irq_pri() {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 200;
	uint8_t ret = false;
	while (!irqTriggeredPri) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?
	if(irqTriggeredPri)
		ret = true;
	else
		kprintf("PATA: IRQ timeout for primary channel!\n");

	// Reset triggered value.
	irqTriggeredPri = false;
	return ret;
}

bool ata_wait_for_irq_sec() {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 200;
	uint8_t ret = false;
	while (!irqTriggeredSec) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?
	if(irqTriggeredSec)
		ret = true;
	else
		kprintf("PATA: IRQ timeout for secondary channel!\n");

	// Reset triggered value.
	irqTriggeredSec = false;
	return ret;
}

uint16_t ata_read_data_word(uint16_t port) {
    return inw(ATA_REG_DATA(port));
}

void ata_send_command(uint16_t portCommand, uint8_t sectorCount, uint8_t sectorNumber, uint8_t cylinderLow, uint8_t cylinderHigh, uint8_t command) {
    // Send command to current ATA device.
    outb(ATA_REG_SECTOR_COUNT(portCommand), sectorCount);
    outb(ATA_REG_SECTOR_NUMBER(portCommand), sectorNumber);
    outb(ATA_REG_CYLINDER_LOW(portCommand), cylinderLow);
    outb(ATA_REG_CYLINDER_HIGH(portCommand), cylinderHigh);
    outb(ATA_REG_COMMAND(portCommand), command);
    io_wait();
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
    io_wait();
}

void ata_soft_reset(uint16_t portCommand, uint16_t portControl, bool *outMasterPresent, bool *outMasterAtapi, bool *outSlavePresent, bool *outSlaveAtapi) {
    // Cycle reset bit.
    outb(ATA_REG_DEVICE_CONTROL(portControl), ATA_DEVICE_CONTROL_RESET);
    sleep(30);
    outb(ATA_REG_DEVICE_CONTROL(portControl), 0x00);
    sleep(500);

    // Select master device.
    ata_select_device(portCommand, portControl, false, true);

    // Get signature.
    uint8_t sectorCount = inb(ATA_REG_SECTOR_COUNT(portCommand));
    uint8_t sectorNumber = inb(ATA_REG_SECTOR_NUMBER(portCommand));
    uint8_t cylinderLow = inb(ATA_REG_CYLINDER_LOW(portCommand));
    uint8_t cylinderHigh = inb(ATA_REG_CYLINDER_HIGH(portCommand));
    inb(ATA_REG_DRIVE_SELECT(portCommand));

    // Check signature of master.
    if (sectorCount == ATA_SIG_SECTOR_COUNT_ATA && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATA
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATA && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATA) {
        *outMasterAtapi = false; // ATA device.
        *outMasterPresent = true;
        kprintf("ATA: Master device on channel 0x%X should be an ATA device.\n", portCommand);
    }
    else if (sectorCount == ATA_SIG_SECTOR_COUNT_ATAPI && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATAPI
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATAPI && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATAPI) {
        *outMasterAtapi = true; // ATAPI device.
        *outMasterPresent = true;
        kprintf("ATA: Master device on channel 0x%X should be an ATAPI device.\n", portCommand);
    }
    else {
        *outMasterAtapi = false;
        *outMasterPresent = false; // Device probably not present.
        kprintf("ATA: Master device on channel 0x%X didn't match known signatures!\n", portCommand);
    }

    // Select slave device.
    ata_select_device(portCommand, portControl, false, false);

    // Get signature.
    sectorCount = inb(ATA_REG_SECTOR_COUNT(portCommand));
    sectorNumber = inb(ATA_REG_SECTOR_NUMBER(portCommand));
    cylinderLow = inb(ATA_REG_CYLINDER_LOW(portCommand));
    cylinderHigh = inb(ATA_REG_CYLINDER_HIGH(portCommand));
    inb(ATA_REG_DRIVE_SELECT(portCommand));

    // Check signature of slave.
    if (sectorCount == ATA_SIG_SECTOR_COUNT_ATA && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATA
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATA && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATA) {
        *outSlaveAtapi = false; // ATA device.
        *outSlavePresent = true;
        kprintf("ATA: Slave device on channel 0x%X should be an ATA device.\n", portCommand);
    }
    else if (sectorCount == ATA_SIG_SECTOR_COUNT_ATAPI && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATAPI
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATAPI && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATAPI) {
        *outSlaveAtapi = true; // ATAPI device.
        *outSlavePresent = true;
        kprintf("ATA: Slave device on channel 0x%X should be an ATAPI device.\n", portCommand);
    }
    else {
        *outSlaveAtapi = false;
        *outSlavePresent = false; // Device probably not present.
        kprintf("ATA: Slave device on channel 0x%X didn't match known signatures!\n", portCommand);
    }
}

bool ata_check_status(uint16_t portCommand, bool master) {
    // Get value of selected drive and ensure it is correct.
    if (master && (inb(ATA_REG_DRIVE_SELECT(portCommand)) & 0x10))
        return false;
    else if (!master && !(inb(ATA_REG_DRIVE_SELECT(portCommand)) & 0x10))
        return false;

    // Ensure status flags are good.
    uint8_t status = inb(ATA_REG_STATUS(portCommand));

    // If error bit is set, print error.
    if (status & ATA_STATUS_ERROR)
        kprintf("ATA: Error 0x%X\n", inb(ATA_REG_ERROR(portCommand)));
    if (status & ATA_STATUS_BUSY)
        kprintf("ATA: Device Busy.\n");

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

void ata_reset_identify(uint16_t portCommand, uint16_t portControl) {
    // Reset channel.
    bool masterPresent = false;
    bool slavePresent = false;
    bool atapiMaster = false;
    bool atapiSlave = false;
    ata_soft_reset(portCommand, portControl, &masterPresent, &atapiMaster, &slavePresent, &atapiSlave);

    // Identify master device if present.
    if (masterPresent) {
        // Select master.
        ata_select_device(portCommand, portControl, false, true);
        if (atapiMaster) { // ATAPI device.
            ata_identify_packet_result_t atapiMaster;
            if (ata_identify_packet(portCommand, portControl, true, &atapiMaster)) {
                kprintf("ATA: Found ATAPI master device on channel 0x%X!\n", portCommand);
                ata_print_device_packet_info(atapiMaster);
            }
            else {
                kprintf("ATA: Failed to identify ATAPI master device on channel 0x%X.\n", portCommand);
            }
        }
        else { // ATA device.
            ata_identify_result_t ataMaster;
            if (ata_identify(portCommand, portControl, true, &ataMaster)) {
                kprintf("ATA: Found master device on channel 0x%X!\n", portCommand);
                ata_print_device_info(ataMaster);
            }
            else {
                kprintf("ATA: Failed to identify master device or no device exists on channel 0x%X.\n", portCommand);
            }
        }
    }
    else { // Master isn't present.
        kprintf("ATA: Master device on channel 0x%X isn't present.\n", portCommand);
    }

    // Identify slave device if present.
    if (slavePresent) {
        // Select slave.
        ata_select_device(portCommand, portControl, false, false);
        if (atapiSlave) { // ATAPI device.
            ata_identify_packet_result_t atapiSlave;
            if (ata_identify_packet(portCommand, portControl, false, &atapiSlave)) {
                kprintf("ATA: Found ATAPI slave device on channel 0x%X!\n", portCommand);
                ata_print_device_packet_info(atapiSlave);
            }
            else {
                kprintf("ATA: Failed to identify ATAPI slave device on channel 0x%X.\n", portCommand);
            }
        }
        else { // ATA device.
            ata_identify_result_t ataSlave;
            if (ata_identify(portCommand, portControl, false, &ataSlave)) {
                kprintf("ATA: Found slave device on channel 0x%X!\n", portCommand);
                ata_print_device_info(ataSlave);
            }
            else {
                kprintf("ATA: Failed to identify slave device or no device exists on channel 0x%X.\n", portCommand);
            }
        }
    }
    else { // Slave isn't present.
        kprintf("ATA: Slave device on channel 0x%X isn't present.\n", portCommand);
    }
}

void ata_init() {
    kprintf("ATA: Initializing...\n");

    // Register interrupt handlers.
    interrupts_irq_install_handler(IRQ_PRI_ATA, ata_callback_pri);
    interrupts_irq_install_handler(IRQ_SEC_ATA, ata_callback_sec);

    // Reset and identify both channels.
    ata_reset_identify(ATA_PRI_COMMAND_PORT, ATA_PRI_CONTROL_PORT);
    ata_reset_identify(ATA_SEC_COMMAND_PORT, ATA_SEC_CONTROL_PORT);

    while(true);
}
