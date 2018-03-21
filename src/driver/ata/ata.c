#include <main.h>
#include <tools.h>
#include <io.h>
#include <kprint.h>
#include <driver/ata/ata.h>
#include <kernel/interrupts/interrupts.h>

static bool irqTriggeredPri = false;

static void pata_callback_pri() {
    irqTriggeredPri = true;
    kprintf("IRQ14 raised!\n");
}

static void pata_wait_for_irq_pri() {
    // Wait until IRQ is triggered or we time out.
    uint16_t timeout = 100;
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

uint16_t pata_read_data_word(uint16_t port) {
    return inw(PATA_CMD_DATA(port));
}

bool pata_identify(uint16_t port) {
    // Send identify command and wait for IRQ.
    outb(PATA_CMD_STATUS_CMD(port), 0xEC);
    pata_wait_for_irq_pri();

    //
    // Read words 0-9.
    //
    uint16_t config = pata_read_data_word(port);    // Config bits.
    uint16_t cylinders = pata_read_data_word(port); // Number of logical cylinders.
    pata_read_data_word(port);                      // Reserved.
    uint16_t heads = pata_read_data_word(port);     // Heads.
    pata_read_data_word(port);                      // Retired.
    pata_read_data_word(port);                      // Retired.
    uint16_t sectorsPerTrack = pata_read_data_word(port); // Sectors per track.
    pata_read_data_word(port);                      // Retired.
    pata_read_data_word(port);                      // Retired.
    pata_read_data_word(port);                      // Retired.

    // Print data.
    kprintf("PATA:    Config word: 0x%X", config);
    kprintf("PATA:    Cylinders: %u", cylinders);
    kprintf("PATA:    Heads: %u", heads);
    kprintf("PATA:    Sectors per track: %u", sectorsPerTrack);

    //
    // Read words 10-19.
    //
    // Read serial number.
    kprintf("PATA:     Serial: ");
    for (uint16_t i = 0; i < 20 / 2; i++) {
        uint16_t serial = pata_read_data_word(port);
        kprintf("%c%c", (char)(serial >> 8), (char)(serial & 0xFF));
    }
    kprintf("\n");
    
    //
    // Read words 20-49.
    //
    pata_read_data_word(port); // Retired.
    pata_read_data_word(port); // Retired.
    pata_read_data_word(port); // Obsolete.
    
    // Read firmware revision.
    kprintf("PATA:     Firmware: ");
    for (uint16_t i = 0; i < 8 / 2; i++) {
        uint16_t firmware = pata_read_data_word(port);
        kprintf("%c%c", (char)(firmware >> 8), (char)(firmware & 0xFF));
    }
    kprintf("\n");

    // Read model.
    kprintf("PATA:     Model: ");
    for (uint16_t i = 0; i < 40 / 2; i++) {
        uint16_t model = pata_read_data_word(port);
        kprintf("%c%c", (char)(model >> 8), (char)(model & 0xFF));
    }
    kprintf("\n");

    // Get words 47 and 49.
    uint8_t maxSectorsInt = (uint8_t)(pata_read_data_word(port)); // Max sectors/int.
    pata_read_data_word(port); // Reserved.
    uint16_t capabilities1 = pata_read_data_word(port); // Capabilities.

    //
    // Reads words 50-59.
    //
    uint16_t capabilities2 = pata_read_data_word(port); // Capabilities.
    uint8_t pioTransferMode = (uint8_t)(pata_read_data_word(port) >> 8); // PIO mode in bits 15-8, 7-0 are retired.
    pata_read_data_word(port); // Retired.
    uint16_t validWords = pata_read_data_word(port); // Valid word flags (bits 0 to 2).
    uint16_t currentCylinders = pata_read_data_word(port); // Number of current logical cylinders.
}

void pata_init() {
    kprintf("PATA: Initializing...\n");

    // Register interrupt handlers.
    interrupts_irq_install_handler(IRQ_PRI_ATA, pata_callback_pri);

    // Clear primary channel's control register, and set the reset bit.
    //outb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT), 0x00);
    //sleep(10);
    outb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT), PATA_DEVICE_CONTROL_RESET);
    io_wait();
    outb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT), 0x00);
    io_wait();

    outb(PATA_CMD_DRIVE_SELECT(PATA_PRI_COMMAND_PORT), 0xA0);
    io_wait();
    inb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT));
    inb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT));
    inb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT));
    inb(PATA_CTRL_DEVICE_ALTSTATUS(PATA_PRI_CONTROL_PORT));

    // Get signature.
    uint8_t sectorCount = inb(PATA_CMD_SECTOR_COUNT(PATA_PRI_COMMAND_PORT));
    uint8_t sectorNum = inb(PATA_CMD_SECTOR_NUM(PATA_PRI_COMMAND_PORT));
    uint8_t cylLow = inb(PATA_CMD_CYL_LOW(PATA_PRI_COMMAND_PORT));
    uint8_t cylHigh = inb(PATA_CMD_CYL_HIGH(PATA_PRI_COMMAND_PORT));
    uint8_t device = inb(PATA_CMD_DRIVE_SELECT(PATA_PRI_COMMAND_PORT));

    // Zero out registers.
    outb(PATA_CMD_SECTOR_COUNT(PATA_PRI_COMMAND_PORT), 0x00);
    outb(PATA_CMD_SECTOR_NUM(PATA_PRI_COMMAND_PORT), 0x00);
    outb(PATA_CMD_CYL_LOW(PATA_PRI_COMMAND_PORT), 0x00);
    outb(PATA_CMD_CYL_HIGH(PATA_PRI_COMMAND_PORT), 0x00);
    io_wait();

    // Send identify.
    kprintf("Sending IDENTIFY...\n");
    pata_identify(PATA_PRI_COMMAND_PORT);
        
    



}
