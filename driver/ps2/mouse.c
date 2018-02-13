#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/mouse.h>
#include <driver/ps2/ps2.h>
#include <kernel/interrupts.h>

// https://wiki.osdev.org/Mouse
// https://wiki.osdev.org/PS/2_Mouse
// http://www.computer-engineering.org/ps2mouse/
// https://houbysoft.com/download/ps2mouse.html

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

// Mouse responses.
enum
{
    PS2_MOUSE_RESPONSE_TEST_PASS    = 0xAA,
    PS2_MOUSE_RESPONSE_ACK          = 0xFA
};

// Mouse types.
enum
{
    PS2_MOUSE_TYPE_STANARD          = 0x00, // Standard PS/2 mouse.
    PS2_MOUSE_TYPE_WHEEL            = 0x03, // Mouse with scroll wheel.
    PS2_MOUSE_TYPE_FIVEBUTTON       = 0x04  // 5-button mouse.
};

// Used for IRQ waiting.
static bool irq_triggered = false;
static uint8_t irq_data;

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
    // Send command to mouse.
    return ps2_send_cmd_data_response(PS2_CMD_WRITE_MOUSE_IN, cmd);
}

void ps2_mouse_connect()
{
    // Set defaults.
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_DEFAULTS);
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_ENABLE_DATA);
}

// Callback for mouse on IRQ12.
static void ps2_mouse_callback(registers_t* regs)
{	
    // Get data.
    uint8_t data = ps2_get_data();

	log("IRQ12 raised! 0x");
    if (!irq_triggered)
    {
        irq_triggered = true;
        irq_data = inb(PS2_DATA_PORT);
    }
   char* tmp;
    log(utoa(data, tmp, 16));
    log("\n");

    // If the data is a self-test complete command, its likely a new mouse connected.
    // If so, initialize it.
    if (data == PS2_MOUSE_RESPONSE_TEST_PASS)
    {
        // Initialize mouse.
        ps2_mouse_connect();
        log("PS/2 mouse installed!\n");   
    }
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

    // Enable mouse port.
    log ("Enabling mouse...\n");
    ps2_send_cmd(PS2_CMD_ENABLE_MOUSEPORT);

    // Register IRQ12 for the mouse.
    interrupts_irq_install_handler(12, ps2_mouse_callback);

    // Enable interrupts from mouse.
    uint8_t config = ps2_send_cmd_response(PS2_CMD_READ_BYTE0) | PS2_CONFIG_MOUSEPORT_INTERRUPT;
    ps2_send_cmd_data(PS2_CMD_WRITE_BYTE0, config);

    // Reset mouse.
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_RESET);
    ps2_mouse_wait_irq();
    sleep(500);

    // Check if mouse is present. If so, set it up.
    if (irq_data == PS2_MOUSE_RESPONSE_ACK || irq_data == PS2_MOUSE_RESPONSE_TEST_PASS)
    {
        ps2_mouse_connect();
        log("PS/2 mouse installed!\n");
    }

    log("PS/2 mouse initialized!\n");
}
