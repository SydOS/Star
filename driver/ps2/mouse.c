#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/mouse.h>
#include <driver/ps2/ps2.h>
#include <kernel/interrupts.h>

// Mouse commands.
enum
{
    PS2_MOUSE_CMD_SET_RESOLUTION    = 0xE8, // Set resolution.
    PS2_MOUSE_CMD_GET_STATUS        = 0xE9, // Get status.
    PS2_MOUSE_CMD_SET_STREAMMODE    = 0xEA, // Set stream mode.
    PS2_MOUSE_CMD_READ_DATA         = 0xEB, // Read data.
    PS2_MOUSE_CMD_RESET_WRAPMODE    = 0xEC, // Reset wrap mode.
    PS2_MOUSE_CMD_SET_WRAPMODE      = 0xEE, // Set wrap mode.
    PS2_MOUSE_CMD_SET_REMOTEMODE    = 0xF0, // Set remote mode.
    PS2_MOUSE_CMD_GET_DEVICEID      = 0xF2, // Get device ID.
    PS2_MOUSE_CMD_SET_SAMPLERATE    = 0xF3, // Set sample rate.
    PS2_MOUSE_CMD_ENABLE_DATA       = 0xF4, // Enable data reporting.
    PS2_MOUSE_CMD_DISABLE_DATA      = 0xF5, // Disable data reporting.
    PS2_MOUSE_CMD_SET_DEFAULTS      = 0xF6, // Restore mouse defaults.
    PS2_MOUSE_CMD_RESEND            = 0xFE, // Resend.
    PS2_MOUSE_CMD_RESET             = 0xFF  // Reset mouse.

};

// Used for IRQ waiting.
static bool irq_triggered = false;

void ps2_mouse_wait_irq()
{
    uint8_t timeout = 300;
    while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(10);
	}
	if(!irq_triggered) { log("MOUSE IRQ TIMEOUT\n"); }
    irq_triggered = false;
}

uint8_t ps2_mouse_send_cmd(uint8_t cmd)
{
    // Prepare to send byte to mouse.
    ps2_send_cmd(PS2_CMD_WRITE_MOUSE_IN);

    // Send command.
    ps2_wait_send();
    outb(PS2_DATA_PORT, cmd);

    // Read result.
    ps2_wait_receive();
    return inb(PS2_DATA_PORT);
}

// Callback for mouse on IRQ12.
static void ps2_mouse_callback(registers_t* regs)
{	
	log("IRQ12 raised! 0x");
    irq_triggered = false;
    
    
    //ps2_mouse_send_cmd(PS2_MOUSE_CMD_READ_DATA);
   // inb(PS2_DATA_PORT);
   char* tmp;

    log(utoa(inb(PS2_DATA_PORT), tmp, 16));
    log("\n");
   // inb(PS2_DATA_PORT);
}

// Initializes the mouse.
void ps2_mouse_init()
{
    // Test port.
    /*if (ps2_send_cmd_response(PS2_CMD_TEST_MOUSEPORT) != PS2_CMD_RESPONSE_PORTTEST_PASS)
    {
        log("Mouse PS/2 port self-test failed!\n");
        return;
    }*/

    // Register IRQ12 for the mouse.
    interrupts_irq_install_handler(12, ps2_mouse_callback);

    // Enable mouse.
    log ("Enabling mouse...\n");
    ps2_send_cmd(PS2_CMD_ENABLE_MOUSEPORT);

    // Enable interrupts from mouse.
    uint8_t config = ps2_read_configuration() | PS2_CONFIG_MOUSEPORT_INTERRUPT;
    ps2_configure(config);

    // Reset mouse and enable data.
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_RESET);
    sleep(500);
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_DEFAULTS);
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_ENABLE_DATA);
    log("PS/2 mouse initialized!\n");
}
