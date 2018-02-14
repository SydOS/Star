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
// http://web.archive.org/web/20021016204550/http://panda.cs.ndsu.nodak.edu:80/~achapwes/PICmicro/mouse/mouse.html

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
    PS2_MOUSE_TYPE_STANDARD         = 0x00, // Standard PS/2 mouse.
    PS2_MOUSE_TYPE_WHEEL            = 0x03, // Mouse with scroll wheel.
    PS2_MOUSE_TYPE_FIVEBUTTON       = 0x04  // 5-button mouse.
};

// Mouse packet masks.
enum
{
    PS2_MOUSE_PACKET_LEFT_BTN       = 0x01,
    PS2_MOUSE_PACKET_RIGHT_BTN      = 0x02,
    PS2_MOUSE_PACKET_MIDDLE_BTN     = 0x04,
    PS2_MOUSE_PACKET_MAGIC          = 0x08
};

#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE1 200
#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE2 100
#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE3 80

// Stores current mouse type.
static uint8_t mouse_type = PS2_MOUSE_TYPE_STANDARD;

uint8_t ps2_mouse_send_cmd(uint8_t cmd)
{
    // Send command to mouse.
    for (uint8_t i = 0; i < 10; i++)
    {
        ps2_send_cmd_response(PS2_CMD_WRITE_MOUSE_IN);
        uint8_t response = ps2_send_data_response(cmd);

        // If the response is valid, return it.
        if (response != PS2_DATA_RESPONSE_RESEND)
            return response;
    }
}

void ps2_mouse_connect()
{
    // Mouse is a standard mouse by default.
    mouse_type = PS2_MOUSE_TYPE_STANDARD;

    // Set defaults.
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_DEFAULTS);
    
    // Send sample rate sequence to test if mouse is an Intellimouse (wheel mouse).
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE1);
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE2);
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_SET_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE3);

    // Query device ID.
    uint8_t mouse_id = ps2_mouse_send_cmd(PS2_MOUSE_CMD_GET_DEVICEID);

    // If ID is an ACK or something, try to get the ID again.
    for (uint8_t i = 0; i < 50; i++)
    {
        if (mouse_id == PS2_DATA_RESPONSE_SELFTEST_PASS || mouse_id == PS2_DATA_RESPONSE_ACK)
            mouse_id = ps2_get_data();
        else
            break;
    }
    
    char* tmp;
    log("PS/2 mouse ID: 0x");
    log(utoa(mouse_id, tmp, 16));
    log("\n");
    if (mouse_id == PS2_MOUSE_TYPE_WHEEL)
    {
        log("Intellimouse detected.\n");
        mouse_type = mouse_id;
    }
    
    ps2_mouse_send_cmd(PS2_MOUSE_CMD_ENABLE_DATA);
    log("PS/2 mouse installed!\n");
}

// Callback for mouse on IRQ12.
static void ps2_mouse_callback(registers_t* regs)
{	
    // Get data.
    uint8_t data = ps2_get_data();

    /*// Diag info.
    char* tmp;
    log(utoa(cycle, tmp, 10));
    log(":");
    log(utoa(data, tmp, 16));
    log("\n");*/

    // Attempt to decode input into mouse packets.
    static uint8_t cycle = 0;
    static uint8_t bad_packet_count = 0;
    static uint8_t mouse_bytes[5];
    mouse_bytes[cycle++] = data;

    // If the data is a self-test complete command, its likely a new mouse connected.
    // If so, initialize it.
    if (data == PS2_MOUSE_RESPONSE_TEST_PASS)
    {
        // Initialize mouse.
        ps2_mouse_connect();
        cycle = 0;
        return;
    }

    // If the data is just an ACK, throw it out.
    if (data == PS2_MOUSE_RESPONSE_ACK)
    {
        cycle = 0;
        return;
    }
  
    // Ensure first byte is valid. If not, reset the packet cycle.
    if ((mouse_bytes[0] & PS2_MOUSE_PACKET_MAGIC) != PS2_MOUSE_PACKET_MAGIC)
    {
        cycle = 0;
        log("Invalid mouse packet!\n");
        bad_packet_count++;
        return;
    }

    // If the bad packet count reaches 3, reset the mouse as something is wrong.
    if (bad_packet_count >= 3)
    {
        bad_packet_count = 0;
        ps2_mouse_send_cmd(PS2_MOUSE_CMD_RESET);
        log("Resetting mouse!\n");
        return;
    }

    // Have we reached the third byte?
    if (mouse_type == PS2_MOUSE_TYPE_STANDARD && cycle == 3)
    {
        // Reset counter.
        cycle = 0;

        if (mouse_bytes[0] & PS2_MOUSE_PACKET_LEFT_BTN)
            log("Left mouse button pressed!\n");
        if (mouse_bytes[0] & PS2_MOUSE_PACKET_RIGHT_BTN)
            log("Right mouse button pressed!\n");
        if (mouse_bytes[0] & PS2_MOUSE_PACKET_MIDDLE_BTN)
            log("Middle mouse button pressed!\n");
    }
    else if (mouse_type == PS2_MOUSE_TYPE_WHEEL && cycle == 4)
    {
        // Reset counter.
        cycle = 0;

        if (mouse_bytes[0] & PS2_MOUSE_PACKET_LEFT_BTN)
            log("Left mouse button pressed!\n");
        if (mouse_bytes[0] & PS2_MOUSE_PACKET_RIGHT_BTN)
            log("Right mouse button pressed!\n");
        if (mouse_bytes[0] & PS2_MOUSE_PACKET_MIDDLE_BTN)
            log("Middle mouse button pressed!\n");
    }
}

// Initializes the mouse.
void ps2_mouse_init()
{
    // Register IRQ12 for the mouse.
    interrupts_irq_install_handler(12, ps2_mouse_callback);

    // Reset mouse.
    ps2_mouse_send_cmd(PS2_DATA_RESET);
    ps2_mouse_connect();
    log("PS/2 mouse initialized!\n");
}
