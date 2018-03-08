#include <main.h>
#include <kprint.h>
#include <tools.h>
#include <driver/ps2/mouse.h>
#include <driver/ps2/ps2.h>
#include <arch/i386/kernel/interrupts.h>

// https://wiki.osdev.org/Mouse
// https://wiki.osdev.org/PS/2_Mouse
// http://www.computer-engineering.org/ps2mouse/
// https://houbysoft.com/download/ps2mouse.html
// http://web.archive.org/web/20021016204550/http://panda.cs.ndsu.nodak.edu:80/~achapwes/PICmicro/mouse/mouse.html

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
    PS2_MOUSE_PACKET_MAGIC          = 0x08,
    PS2_MOUSE_PACKET_SIGN_X         = 0x10,
    PS2_MOUSE_PACKET_SIGN_Y         = 0x20,
    PS2_MOUSE_PACKET_OVERFLOW_X     = 0x40,
    PS2_MOUSE_PACKET_OVERFLOW_Y     = 0x80,

    PS2_MOUSE_PACKET_FOURTH_BTN     = 0x10,
    PS2_MOUSE_PACKET_FIFTH_BTN      = 0x20,
    PS2_MOUSE_PACKET_MAGIC_BIT6     = 0x40,
    PS2_MOUSE_PACKET_MAGIC_BIT7     = 0x80
};

#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE1 200
#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE2 100
#define PS2_MOUSE_INTELLIMOUSE_SAMPLERATE3 80
#define PS2_MOUSE_FIVEBUTTON_SAMPLERATE1 200
#define PS2_MOUSE_FIVEBUTTON_SAMPLERATE2 200
#define PS2_MOUSE_FIVEBUTTON_SAMPLERATE3 80

// Stores current mouse type.
static uint8_t mouse_type = PS2_MOUSE_TYPE_STANDARD;

// Sends a command to the mouse.
uint8_t ps2_mouse_send_cmd(uint8_t cmd)
{
    // Send command to mouse.
    uint8_t response = 0;
    for (uint8_t i = 0; i < 10; i++)
    {
        ps2_send_cmd_response(PS2_CMD_WRITE_MOUSE_IN);
        response = ps2_send_data_response(cmd);

        // If the response is valid, return it.
        if (response != PS2_DATA_RESPONSE_RESEND)
            break;
    }

    // Return response.
    return response;
}

// Initializes a mouse.
void ps2_mouse_connect(bool from_irq)
{
    if (from_irq)
    {
        // Read the current configuration byte.
        uint8_t config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
        //kprintf("Initial PS/2 configuration byte: 0x%X\n", config);

        // Disable IRQse.
        config &= ~(PS2_CONFIG_ENABLE_KEYBPORT_INTERRUPT | PS2_CONFIG_ENABLE_MOUSEPORT_INTERRUPT);

        // Write config to controller.
        ps2_send_cmd(PS2_CMD_WRITE_BYTE);
        ps2_send_data(config);
        config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
        //kprintf("New PS/2 configuration byte: 0x%X\n", config);
    }

    // Mouse is a standard mouse by default.
    mouse_type = PS2_MOUSE_TYPE_STANDARD;

    // Set defaults.
    ps2_mouse_send_cmd(PS2_DATA_SET_DEFAULTS);
    
    // Send sample rate sequence to test if mouse is an Intellimouse (wheel mouse).
    ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE1);
    ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE2);
    ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
    ps2_mouse_send_cmd(PS2_MOUSE_INTELLIMOUSE_SAMPLERATE3);

    // Query device ID.
    uint8_t mouse_id = ps2_mouse_send_cmd(PS2_DATA_IDENTIFY);

    // If ID is an ACK or something, try to get the ID again.
    for (uint8_t i = 0; i < 50; i++)
    {
        if (mouse_id == PS2_DATA_RESPONSE_SELFTEST_PASS || mouse_id == PS2_DATA_RESPONSE_ACK)
            mouse_id = ps2_get_data();
        else
            break;
    }
    
    if (mouse_id == PS2_MOUSE_TYPE_WHEEL)
    {
        kprintf("Intellimouse detected.\n");
        mouse_type = mouse_id;

        // Attempt to enter 5-button mode.
        // Send sample rate sequence to test if mouse is a 5-button mouse.
        ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
        ps2_mouse_send_cmd(PS2_MOUSE_FIVEBUTTON_SAMPLERATE1);
        ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
        ps2_mouse_send_cmd(PS2_MOUSE_FIVEBUTTON_SAMPLERATE2);
        ps2_mouse_send_cmd(PS2_DATA_SET_MOUSE_SAMPLERATE);
        ps2_mouse_send_cmd(PS2_MOUSE_FIVEBUTTON_SAMPLERATE3);

        // Query device ID.
        mouse_id = ps2_mouse_send_cmd(PS2_DATA_IDENTIFY);

        // If ID is an ACK or something, try to get the ID again.
        for (uint8_t i = 0; i < 50; i++)
        {
            if (mouse_id == PS2_DATA_RESPONSE_SELFTEST_PASS || mouse_id == PS2_DATA_RESPONSE_ACK)
                mouse_id = ps2_get_data();
            else
                break;
        }

        if (mouse_id == PS2_MOUSE_TYPE_FIVEBUTTON)
        {
            kprintf("5-button mouse detected.\n");
            mouse_type = mouse_id;
        }
    }

    if (from_irq)
    {
        // Read the current configuration byte.
        uint8_t config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
        //kprintf("Initial PS/2 configuration byte: 0x%X\n", config);

        // Enable interrupts.
        config |= PS2_CONFIG_ENABLE_KEYBPORT_INTERRUPT | PS2_CONFIG_ENABLE_MOUSEPORT_INTERRUPT;

        // Write config to controller.
        ps2_send_cmd(PS2_CMD_WRITE_BYTE);
        ps2_send_data(config);
        config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
        //kprintf("New PS/2 configuration byte: 0x%X\n", config);
    }

    // If the ID is 0xFE at this point, likely no mouse is present.
    if (mouse_id == PS2_DATA_RESPONSE_RESEND)
    {
        kprintf("A working mouse couldn't be found!\n");
        return;
    } 

    kprintf("PS/2 mouse ID: 0x%X\n", mouse_id);
    
    ps2_mouse_send_cmd(PS2_DATA_ENABLE);
    kprintf("PS/2 mouse installed!\n");
}

// Processes a packet.
static void ps2_mouse_process_packet(uint8_t mouse_bytes[], uint8_t packet_size)
{
    // If either overflow value is set, ignore it.
    if (mouse_bytes[0] & PS2_MOUSE_PACKET_OVERFLOW_X || mouse_bytes[0] & PS2_MOUSE_PACKET_OVERFLOW_Y)
        return;

    // Get X and Y.
    int16_t x = ((mouse_bytes[0] & PS2_MOUSE_PACKET_SIGN_X) ? 0xFFFFFF00 : 0) | mouse_bytes[1];
    int16_t y = -(((mouse_bytes[0] & PS2_MOUSE_PACKET_SIGN_Y) ? 0xFFFFFF00 : 0) | mouse_bytes[2]);
    int16_t z = 0;

    // Get mouse wheel.
    if (packet_size == 4)
    {   
        if (mouse_type == PS2_MOUSE_TYPE_FIVEBUTTON)
        {
            // Check last two bits of last packet. If not zero, throw packet out.
            if (mouse_bytes[3] & PS2_MOUSE_PACKET_MAGIC_BIT6 || mouse_bytes[3] & PS2_MOUSE_PACKET_MAGIC_BIT7)
                return;

            if (mouse_bytes[3] & PS2_MOUSE_PACKET_FOURTH_BTN)
                kprintf("Fourth mouse button pressed!\n");
            if (mouse_bytes[3] & PS2_MOUSE_PACKET_FIFTH_BTN)
                kprintf("Fifth mouse button pressed!\n");
        }

        // Get Z-value (wheel).
        z = (int16_t)((((int8_t)(mouse_bytes[3] << 4)) >> 4));
    }

    // If we get values of (-)240 or higher/lower, discard.
    if (x > 240 || x < -240 || y > 240 || y < -240 || z > 240 || z < -240)
        return;

    if (mouse_bytes[0] & PS2_MOUSE_PACKET_LEFT_BTN)
        kprintf("Left mouse button pressed!\n");
    if (mouse_bytes[0] & PS2_MOUSE_PACKET_RIGHT_BTN)
        kprintf("Right mouse button pressed!\n");
    if (mouse_bytes[0] & PS2_MOUSE_PACKET_MIDDLE_BTN)
        kprintf("Middle mouse button pressed!\n");

    // Print status.
    if (x != 0 || y != 0 || z != 0)
        kprintf("Mouse moved: X: %i Y: %i Z: %i\n", x, y, z);
}

// Callback for mouse on IRQ12.
static void ps2_mouse_callback()
{	
    // Get data.
    uint8_t data = ps2_get_data();
    static uint8_t last_data = 0;

    // Attempt to decode input into mouse packets.
    static uint8_t cycle = 0;
    static uint8_t mouse_bytes[4];
    mouse_bytes[cycle++] = data;

    // If the data is a self-test complete command, it's likely a new mouse connected.
    if (last_data == PS2_DATA_RESPONSE_SELFTEST_PASS && data == PS2_MOUSE_TYPE_STANDARD)
    {
        // Initialize mouse.
        last_data = data;
        ps2_mouse_connect(true);
        cycle = 0;
        return;
    }

    // Save data.
    last_data = data;

    // If the data is just an ACK, throw it out.
    if (data == PS2_DATA_RESPONSE_ACK)
    {
        cycle = 0;
        return;
    }

    // Ensure first byte is valid. If not, reset the packet cycle.
    if ((mouse_bytes[0] & PS2_MOUSE_PACKET_MAGIC) != PS2_MOUSE_PACKET_MAGIC)
    {
        cycle = 0;
        return;
    }

    // Have we reached the final byte?
    if ((mouse_type == PS2_MOUSE_TYPE_STANDARD && cycle == 3) || (mouse_type == PS2_MOUSE_TYPE_WHEEL && cycle == 4) ||
        (mouse_type == PS2_MOUSE_TYPE_FIVEBUTTON && cycle == 4))
    {
        // Process packet and reset cycle.
        ps2_mouse_process_packet(mouse_bytes, cycle);
        cycle = 0;
    }
}

// Initializes the mouse.
void ps2_mouse_init()
{
    // Register IRQ12 for the mouse.
    interrupts_irq_install_handler(12, ps2_mouse_callback);

    // Reset mouse.
    ps2_mouse_send_cmd(PS2_DATA_RESET);
    ps2_mouse_connect(false);
    kprintf("PS/2 mouse initialized!\n");
}
