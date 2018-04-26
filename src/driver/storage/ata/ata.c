#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/storage/ata/ata.h>
#include <driver/storage/ata/ata_commands.h>
#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>
#include <driver/storage/storage.h>
#include <driver/pci.h>

ata_channel_t *isaPrimary;
ata_channel_t *isaSecondary;

int16_t ata_check_status(ata_channel_t *channel, bool master) {
    // Get value of selected drive and ensure it is correct.
    if (master && (inb(ATA_REG_DRIVE_SELECT(channel->CommandPort)) & 0x10))
        return ATA_CHK_STATUS_DEVICE_MISMATCH;
    else if (!master && !(inb(ATA_REG_DRIVE_SELECT(channel->CommandPort)) & 0x10))
        return ATA_CHK_STATUS_DEVICE_MISMATCH;

    // Get status flag. If status is not ready, return error.
    uint8_t status = inb(ATA_REG_STATUS(channel->CommandPort));
    if (status & ATA_STATUS_ERROR)
        return inb(ATA_REG_ERROR(channel->CommandPort));
    else if (status & ATA_STATUS_DRIVE_FAULT)
        return ATA_CHK_STATUS_DEVICE_FAULT;
    else if ((status & ATA_STATUS_BUSY) || !(status & ATA_STATUS_READY))
        return ATA_CHK_STATUS_DRIVE_BUSY;

    // Drive is OK and ready.
    return ATA_CHK_STATUS_OK;
}

static bool ata_callback_isa(irq_regs_t *regs, uint8_t irqNum) {
    kprintf("ATA: ISA IRQ%u raised!\n", irqNum);
    if (irqNum == IRQ_PRI_ATA)
        isaPrimary->InterruptTriggered = true;
    else if (irqNum == IRQ_SEC_ATA)
        isaSecondary->InterruptTriggered = true;
    return true;
}

static bool ata_callback_pci(pci_device_t *device) {
    // Is the interrupt bit in the PCI status register set?
    if (pci_config_read_word(device, PCI_REG_STATUS) & ATA_PCI_STATUS_INTERRUPT) {
        kprintf("ATA: PCI interrupt raised!\n");
        ata_device_t *ataDevice = (ata_device_t*)device->DriverObject;

        // Is busmastering enabled on primary channel?
        if (ataDevice->Primary.BusMasterCapable) {
            // Check if interrupt bit is set.
            if (inb(ataDevice->Primary.BusMasterStatusPort) & ATA_PCI_BUSMASTER_STATUS_INTERRUPT)
                ataDevice->Primary.InterruptTriggered = true;

            // Reset interrupt bit.
            outb(ataDevice->Primary.BusMasterStatusPort, ATA_PCI_BUSMASTER_STATUS_INTERRUPT);
        }

        // Is busmastering enabled on secondary channel?
        if (ataDevice->Secondary.BusMasterCapable) {
            // Check if interrupt bit is set.
            if (inb(ataDevice->Secondary.BusMasterStatusPort) & ATA_PCI_BUSMASTER_STATUS_INTERRUPT)
                ataDevice->Secondary.InterruptTriggered = true;

            // Reset interrupt bit.
            outb(ataDevice->Secondary.BusMasterStatusPort, ATA_PCI_BUSMASTER_STATUS_INTERRUPT);
        }

        // Interrupt is handled.
        return true;
    }

    // Device didn't have interrupt bit set, interrupt not handled.
    return false;
}

int16_t ata_wait_for_irq(ata_channel_t *channel, bool master) {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 200;
	bool ret = false;
	while (!channel->InterruptTriggered) {
		if(!timeout)
			break;
		timeout--;
		sleep(10);
	}

	// Did we hit the IRQ?

	if(channel->InterruptTriggered)
		ret = true;
	else
		kprintf("ATA: IRQ timeout for channel 0x%X!\n", channel->CommandPort);

	// Reset triggered value.
	channel->InterruptTriggered = false;
	if (ret)
        return ata_check_status(channel, master);
    else
        return -1;
}

uint16_t ata_read_data_word(uint16_t portCommand) {
    return inw(ATA_REG_DATA(portCommand));
}

void ata_read_data_pio(ata_channel_t *channel, void *outData, uint32_t size) {
    // Get pointer to word array.
    uint16_t *buffer = (uint16_t*)outData;

    // Read words from device.
    for (uint32_t i = 0; i < size; i += 2)
        buffer[i / 2] = inw(ATA_REG_DATA(channel->CommandPort));
}

void ata_write_data_pio(ata_channel_t *channel, const void *data, uint32_t size) {
    // Get pointer to word array.
    uint16_t *buffer = (uint16_t*)data;

    // Write words to device.
    for (uint32_t i = 0; i < size; i += 2)
        outw(ATA_REG_DATA(channel->CommandPort), buffer[i / 2]);
}

void ata_send_params(ata_channel_t *channel, uint8_t sectorCount, uint8_t sectorNumber, uint8_t cylinderLow, uint8_t cylinderHigh) {
    // Send params to ATA device.
    outb(ATA_REG_SECTOR_COUNT(channel->CommandPort), sectorCount);
    outb(ATA_REG_SECTOR_NUMBER(channel->CommandPort), sectorNumber);
    outb(ATA_REG_CYLINDER_LOW(channel->CommandPort), cylinderLow);
    outb(ATA_REG_CYLINDER_HIGH(channel->CommandPort), cylinderHigh);
    io_wait();
}

void ata_send_command(ata_channel_t *channel, uint8_t sectorCount, uint8_t sectorNumber, uint8_t cylinderLow, uint8_t cylinderHigh, uint8_t command) {
    // Send command to ATA device.
    ata_send_params(channel, sectorCount, sectorNumber, cylinderLow, cylinderHigh);
    outb(ATA_REG_COMMAND(channel->CommandPort), command);
    io_wait();
}

void ata_set_lba_high(ata_channel_t *channel, uint8_t lbaHigh) {
    // Get current device value.
    uint8_t deviceFlags = inb(ATA_REG_DRIVE_SELECT(channel->CommandPort));

    // Set LBA flag and add high bits of LBA address, and write back to register.
    deviceFlags |= ATA_DEVICE_FLAGS_LBA | lbaHigh;
    outb(ATA_REG_DRIVE_SELECT(channel->CommandPort), deviceFlags);
    io_wait();
}

void ata_select_device(ata_channel_t *channel, bool master) {
    uint8_t deviceFlags = ATA_DEVICE_FLAGS_ECC | ATA_DEVICE_FLAGS_SECTOR;
    if (!master)
        deviceFlags |= ATA_DEVICE_FLAGS_SLAVE;
    outb(ATA_REG_DRIVE_SELECT(channel->CommandPort), deviceFlags);
    io_wait();
    inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    io_wait();
}

void ata_soft_reset(ata_channel_t *channel, bool *outMasterPresent, bool *outMasterAtapi, bool *outSlavePresent, bool *outSlaveAtapi) {
    // Cycle reset bit.
    outb(ATA_REG_DEVICE_CONTROL(channel->ControlPort), ATA_DEVICE_CONTROL_RESET);
    sleep(10);
    outb(ATA_REG_DEVICE_CONTROL(channel->ControlPort), 0x00);
    sleep(500);

    // Select master device.
    ata_select_device(channel, true);

    // Get signature.
    uint8_t sectorCount = inb(ATA_REG_SECTOR_COUNT(channel->CommandPort));
    uint8_t sectorNumber = inb(ATA_REG_SECTOR_NUMBER(channel->CommandPort));
    uint8_t cylinderLow = inb(ATA_REG_CYLINDER_LOW(channel->CommandPort));
    uint8_t cylinderHigh = inb(ATA_REG_CYLINDER_HIGH(channel->CommandPort));
    inb(ATA_REG_DRIVE_SELECT(channel->CommandPort));

    // Check signature of master.
    kprintf("ATA: SC: 0x%X SN: 0x%X CL: 0x%X CH: 0x%X\n", sectorCount, sectorNumber, cylinderLow, cylinderHigh);
    if (sectorCount == ATA_SIG_SECTOR_COUNT_ATA && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATA
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATA && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATA) {
        *outMasterAtapi = false; // ATA device.
        *outMasterPresent = true;
        kprintf("ATA: Master device on channel 0x%X should be an ATA device.\n", channel->CommandPort);
    }
    else if (sectorCount == ATA_SIG_SECTOR_COUNT_ATAPI && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATAPI
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATAPI && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATAPI) {
        *outMasterAtapi = true; // ATAPI device.
        *outMasterPresent = true;
        kprintf("ATA: Master device on channel 0x%X should be an ATAPI device.\n", channel->CommandPort);
    }
    else {
        *outMasterAtapi = false;
        *outMasterPresent = false; // Device probably not present.
        kprintf("ATA: Master device on channel 0x%X didn't match known signatures!\n", channel->CommandPort);
    }

    // Select slave device.
    ata_select_device(channel, false);

    // Get signature.
    sectorCount = inb(ATA_REG_SECTOR_COUNT(channel->CommandPort));
    sectorNumber = inb(ATA_REG_SECTOR_NUMBER(channel->CommandPort));
    cylinderLow = inb(ATA_REG_CYLINDER_LOW(channel->CommandPort));
    cylinderHigh = inb(ATA_REG_CYLINDER_HIGH(channel->CommandPort));
    inb(ATA_REG_DRIVE_SELECT(channel->CommandPort));

    // Check signature of slave.
    kprintf("ATA: SC: 0x%X SN: 0x%X CL: 0x%X CH: 0x%X\n", sectorCount, sectorNumber, cylinderLow, cylinderHigh);
    if (sectorCount == ATA_SIG_SECTOR_COUNT_ATA && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATA
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATA && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATA) {
        *outSlaveAtapi = false; // ATA device.
        *outSlavePresent = true;
        kprintf("ATA: Slave device on channel 0x%X should be an ATA device.\n", channel->CommandPort);
    }
    else if (sectorCount == ATA_SIG_SECTOR_COUNT_ATAPI && sectorNumber == ATA_SIG_SECTOR_NUMBER_ATAPI
        && cylinderLow == ATA_SIG_CYLINDER_LOW_ATAPI && cylinderHigh == ATA_SIG_CYLINDER_HIGH_ATAPI) {
        *outSlaveAtapi = true; // ATAPI device.
        *outSlavePresent = true;
        kprintf("ATA: Slave device on channel 0x%X should be an ATAPI device.\n", channel->CommandPort);
    }
    else {
        *outSlaveAtapi = false;
        *outSlavePresent = false; // Device probably not present.
        kprintf("ATA: Slave device on channel 0x%X didn't match known signatures!\n", channel->CommandPort);
    }
}

bool ata_wait_for_drq(ata_channel_t *channel) {
    uint16_t timeout = 500;
    uint8_t response = inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    sleep(1);

    // Is the drive busy?
    while ((response & ATA_STATUS_BUSY) || ((response & ATA_STATUS_DATA_REQUEST) == 0)) {
        if (!timeout) {
            kprintf("ATA: Timeout waiting for device!\n");
            return false;
        }

        // Decrement timeout period and try again.
        timeout--;
        sleep(10);
        response = inb(ATA_REG_ALT_STATUS(channel->ControlPort));
    }

    // Drive is ready.
    return true;
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
    if (info.commandFlags83.info.lba48BitSupported)
        kprintf("ATA:    48-bit LBA supported.\n");

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
    kprintf("ATA:    Device type: 0x%X\n", info.generalConfig.info.deviceType);
    kprintf("ATA:    Packet type: 0x%X\n", info.generalConfig.info.packetSize);

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

void ata_reset_identify(ata_channel_t *channel) {
    // Reset channel.
    bool masterPresent = false;
    bool slavePresent = false;
    bool atapiMaster = false;
    bool atapiSlave = false;
    ata_soft_reset(channel, &masterPresent, &atapiMaster, &slavePresent, &atapiSlave);

    // Identify master device if present.
    if (masterPresent) {
        // Select master.
        ata_select_device(channel, true);
        if (atapiMaster) { // ATAPI device.
            /*ata_identify_packet_result_t atapiMaster;
            if (ata_identify_packet(portCommand, portControl, true, &atapiMaster)) {
                kprintf("ATA: Found ATAPI master device on channel 0x%X!\n", portCommand);
                ata_print_device_packet_info(atapiMaster);
            }
            else {
                kprintf("ATA: Failed to identify ATAPI master device on channel 0x%X.\n", portCommand);
            }*/
        }
        else { // ATA device.
            ata_identify_result_t ataMaster;
            if (ata_identify(channel, true, &ataMaster) == ATA_CHK_STATUS_OK) {
                kprintf("ATA: Found master device on channel 0x%X!\n", channel->CommandPort);
                ata_print_device_info(ataMaster);
            }
            else {
                kprintf("ATA: Failed to identify master device or no device exists on channel 0x%X.\n", channel->CommandPort);
            }
        }
    }
    else { // Master isn't present.
        kprintf("ATA: Master device on channel 0x%X isn't present.\n", channel->CommandPort);
    }

    // Identify slave device if present.
   /* if (slavePresent) {
        // Select slave.
        ata_select_device(portCommand, portControl, false);
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
    }*/
}

void ata_init(pci_device_t *device) {
  /*  kprintf("ATA: Initializing controller on PCI bus %u device %u function %u...\n", device->Bus, device->Device, device->Function);

    // Create ATA device object.
    ata_device_t *ataDevice = (ata_device_t*)kheap_alloc(sizeof(ata_device_t));
    memset(ataDevice, 0, sizeof(ata_device_t));
    device->DriverObject = ataDevice;
    device->InterruptHandler = ata_callback_pci;

    // Get contents of programming interface.
    uint8_t pi = pci_config_read_byte(device, PCI_REG_PROG_IF);

    // Change into native mode if possible.
    if (pi & ATA_PCI_PIF_PRI_SUPPORTS_NATIVE)
        pi |= ATA_PCI_PIF_PRI_NATIVE_MODE;
    if (pi & ATA_PCI_PIF_SEC_SUPPORTS_NATIVE)
        pi |= ATA_PCI_PIF_SEC_NATIVE_MODE;
    pci_config_write_byte(device, PCI_REG_PROG_IF, pi);
    pi = pci_config_read_byte(device, PCI_REG_PROG_IF);

    // Get info.
    kprintf("ATA: Primary channel mode: %s\n", pi & ATA_PCI_PIF_PRI_NATIVE_MODE ? "native" : "compatibility");
    kprintf("ATA: Secondary channel mode: %s\n", pi & ATA_PCI_PIF_SEC_NATIVE_MODE ? "native" : "compatibility");

    // Get primary channel ports.
    if ((pi & ATA_PCI_PIF_PRI_NATIVE_MODE) && (device->BAR[0] & PCI_BAR_PORT_MASK) && (device->BAR[1] & PCI_BAR_PORT_MASK)) {
        ataDevice->Primary.CommandPort = (uint16_t)(device->BAR[0] & PCI_BAR_PORT_MASK);
        ataDevice->Primary.ControlPort = (uint16_t)(device->BAR[1] & PCI_BAR_PORT_MASK);
        ataDevice->Primary.Interrupt = device->InterruptLine;
    }
    else {
        // Use default ISA ports.
        ataDevice->Primary.CommandPort = ATA_PRI_COMMAND_PORT;
        ataDevice->Primary.ControlPort = ATA_PRI_CONTROL_PORT;
        ataDevice->Primary.Interrupt = IRQ_PRI_ATA;
        isaPrimary = &ataDevice->Primary;
        irqs_install_handler(IRQ_PRI_ATA, ata_callback_isa);
    }
    ataDevice->Primary.InterruptTriggered = false;

    // Get secondary channel ports.
    if ((pi & ATA_PCI_PIF_SEC_NATIVE_MODE) && (device->BAR[2] & PCI_BAR_PORT_MASK) && (device->BAR[3] & PCI_BAR_PORT_MASK)) {
        ataDevice->Secondary.CommandPort = (uint16_t)(device->BAR[2] & PCI_BAR_PORT_MASK);
        ataDevice->Secondary.ControlPort = (uint16_t)(device->BAR[3] & PCI_BAR_PORT_MASK);
        ataDevice->Secondary.Interrupt = device->InterruptLine;
    }
    else {
        // Use default ISA ports.
        ataDevice->Secondary.CommandPort = ATA_SEC_COMMAND_PORT;
        ataDevice->Secondary.ControlPort = ATA_SEC_CONTROL_PORT;
        ataDevice->Secondary.Interrupt = IRQ_SEC_ATA;
        isaSecondary = &ataDevice->Secondary;
        irqs_install_handler(IRQ_SEC_ATA, ata_callback_isa);
    }
    ataDevice->Secondary.InterruptTriggered = false;

    // Print ports.
    kprintf("ATA: Primary channel ports: 0x%X and 0x%X\n", ataDevice->Primary.CommandPort, ataDevice->Primary.ControlPort);
    kprintf("ATA: Secondary channel ports: 0x%X and 0x%X\n", ataDevice->Secondary.CommandPort, ataDevice->Secondary.ControlPort);
    kprintf("ATA: Primary channel IRQ: IRQ%u, Secondary: IRQ%u\n", ataDevice->Primary.Interrupt, ataDevice->Secondary.Interrupt);

    // Get bus master info.
    if ((pi & ATA_PCI_PIF_BUSMASTER) && device->BaseAddresses[4].PortMapped) {
        kprintf("ATA: Bus Master supported.\n");
        ataDevice->Primary.BusMasterCapable = true;
        ataDevice->Secondary.BusMasterCapable = true;

        // Get busmaster ports.
        uint16_t busMasterBase = (uint16_t)(device->BAR[4] & PCI_BAR_PORT_MASK);
        ataDevice->Primary.BusMasterCommandPort = busMasterBase + ATA_PCI_BUSMASTER_PORT_PRICMD;
        ataDevice->Primary.BusMasterStatusPort = busMasterBase + ATA_PCI_BUSMASTER_PORT_PRISTATUS;
        ataDevice->Primary.BusMasterPrdt = busMasterBase + ATA_PCI_BUSMASTER_PORT_PRIPRDT;
        ataDevice->Secondary.BusMasterCommandPort = busMasterBase + ATA_PCI_BUSMASTER_PORT_SECCMD;
        ataDevice->Secondary.BusMasterStatusPort = busMasterBase + ATA_PCI_BUSMASTER_PORT_SECSTATUS;
        ataDevice->Secondary.BusMasterPrdt = busMasterBase + ATA_PCI_BUSMASTER_PORT_SECPRDT;
    }

    // Reset and identify both channels.
    kprintf("ATA: Resetting channels...\n");
    ata_reset_identify(&ataDevice->Primary);
    ata_reset_identify(&ataDevice->Secondary);
    kprintf("ATA: Initialized!\n");
*/


    // ===== DEMO ======
    // Select masters on channels.
  /*  kprintf("ATA: Selecting masters....\n");
    ata_select_device(&ataDevice->Primary, true);
    ata_select_device(&ataDevice->Secondary, true);
    uint8_t *data = (uint8_t*)kheap_alloc(ATA_SECTOR_SIZE_512);

    // Wipe first sector of primary master.
    kprintf("ATA: Wiping sector 0 on pri master....\n");
    memset(data, 0, ATA_SECTOR_SIZE_512);
    int16_t status = ata_write_sector(&ataDevice->Primary, true, 0, data, 1);
    kprintf("ATA: Status %d\n", status);
    status = ata_read_sector(&ataDevice->Primary, true, 0, data, 1);
    kprintf("ATA: Status %d\n", status);
    for (int i = 0; i < 25; i++)
        kprintf("%X ", data[i]);
    kprintf("\n\n");

    // Read data from secondary master and write to primary master.
    kprintf("ATA: Copying sector 0 from sec master to pri master....\n");
    status = ata_read_sector(&ataDevice->Secondary, true, 0, data, 1);
    kprintf("ATA: Status %d\n", status);
    status = ata_write_sector(&ataDevice->Primary, true, 0, data, 1);
    kprintf("ATA: Status %d\n", status);
    status = ata_read_sector(&ataDevice->Primary, true, 0, data, 1);
    kprintf("ATA: Status %d\n", status);
    for (int i = 0; i < 25; i++)
        kprintf("%X ", data[i]);
    kprintf("\n\n");*/
}
