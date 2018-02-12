#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/ps2.h>
#include <driver/ps2/keyboard.h>
#include <driver/ps2/mouse.h>

// Ports used for comms with controller.
#define PS2_REG_PORT    0x64

void ps2_send_cmd(uint8_t cmd)
{
    // Send command to PS/2 controller.
    outb(PS2_REG_PORT, cmd);
}

uint8_t ps2_send_cmd_response(uint8_t cmd)
{
    // Send command to PS/2 controller.
    ps2_send_cmd(cmd);

    // Wait for and get response.
    while(!(inb(PS2_REG_PORT) & PS2_STATUS_OUTPUTBUFFERFULL));
    return inb(PS2_DATA_PORT);
}

uint8_t ps2_read_configuration()
{
    return ps2_send_cmd_response(PS2_CMD_READ_BYTE0);
}

void ps2_configure(uint8_t configuration)
{
    //Send configuration.
    ps2_send_cmd(PS2_CMD_WRITE_BYTE0);
    while((inb(PS2_REG_PORT) & PS2_STATUS_INPUTBUFFERFULL));
    outb(PS2_DATA_PORT, configuration);
}

void ps2_flush()
{
    // Flush PS/2 buffer.
    while(inb(PS2_REG_PORT) & PS2_STATUS_OUTPUTBUFFERFULL)
        inb(PS2_DATA_PORT);
}

void ps2_init()
{
    // Disable PS/2 devices.
    ps2_send_cmd(PS2_CMD_DISABLE_KEYBPORT);
    ps2_send_cmd(PS2_CMD_DISABLE_MOUSEPORT);

    // Flush buffer.
    ps2_flush();

    // Perform test of the PS/2 controller. NOTE: seems to kill PS/2 on some hardware.
   /* if (ps2_send_cmd_response(PS2_CMD_SELFTEST) != PS2_CMD_RESPONSE_SELFTEST_PASS)
    {
        log("PS/2 controller self-test failed!\n");
        return;
    }*/

    // Hand it off to devices here.
    ps2_keyboard_init();
    ps2_mouse_init();

    log("PS/2 controller initialized!\n");
}
