#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/ps2.h>
#include <driver/ps2/keyboard.h>
#include <driver/ps2/mouse.h>

// Ports used for comms with controller.


void ps2_wait_send()
{
    // Input buffer must be clear before sending data.
    uint32_t timeout = 10000;
    while (timeout--)
        if((inb(PS2_CMD_PORT) & PS2_STATUS_INPUTBUFFERFULL) == 0)
            return;
}

void ps2_wait_receive()
{
    // Output buffer must be set before we can get data.
    uint32_t timeout = 10000;
    while (timeout--)
        if((inb(PS2_CMD_PORT) & PS2_STATUS_OUTPUTBUFFERFULL) == 1)
            return;
}

void ps2_send_cmd(uint8_t cmd)
{
    // Send command to PS/2 controller.
    ps2_wait_send();
    outb(PS2_CMD_PORT, cmd);
}

uint8_t ps2_send_cmd_response(uint8_t cmd)
{
    // Flush PS/2 buffer.
    while(inb(PS2_CMD_PORT) & PS2_STATUS_OUTPUTBUFFERFULL)
        inb(PS2_DATA_PORT);

    // Send command to PS/2 controller.
    ps2_send_cmd(cmd);

    // Wait for and get response.
    ps2_wait_receive();
    return inb(PS2_DATA_PORT);
}

void ps2_send_data(uint8_t data)
{
    // Send data packet.
    ps2_wait_send();
    outb(PS2_DATA_PORT, data);
}

uint8_t ps2_send_data_response(uint8_t data)
{
    // Flush PS/2 buffer.
    while(inb(PS2_CMD_PORT) & PS2_STATUS_OUTPUTBUFFERFULL)
        inb(PS2_DATA_PORT);

    // Send data to PS/2 device.
    ps2_send_data(data);

    // Wait for and get response.
    ps2_wait_receive();
    return inb(PS2_DATA_PORT);
}

uint8_t ps2_get_data()
{
    // Wait for and get response.
    ps2_wait_receive();
    return inb(PS2_DATA_PORT);
}

uint8_t ps2_get_status()
{
    // Return status register.
    return inb(PS2_CMD_PORT);
}

void ps2_init()
{
    // Disable ports.
    ps2_send_cmd(PS2_CMD_DISABLE_KEYBPORT);
    ps2_send_cmd(PS2_CMD_DISABLE_MOUSEPORT);

    // Flush PS/2 buffer.
    while(inb(PS2_CMD_PORT) & PS2_STATUS_OUTPUTBUFFERFULL)
        inb(PS2_DATA_PORT);

    // Enable ports.
    ps2_send_cmd(PS2_CMD_ENABLE_MOUSEPORT);
    ps2_send_cmd(PS2_CMD_ENABLE_KEYBPORT);

    // Read the current configuration byte.
    uint8_t config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
    char* tmp;
    log("Initial PS/2 configuration byte: 0x");
    log(utoa(config, tmp, 16));
    log("\n");

    // Perform test of the PS/2 controller.
    uint8_t test_byte = ps2_send_cmd_response(PS2_CMD_TEST_CONTROLLER);
    for (uint8_t i = 0; i < 10; i++)
    {
        // Check and re-test if needed.
        if(test_byte == PS2_CMD_RESPONSE_SELFTEST_PASS)
            break;
        test_byte = ps2_send_cmd_response(PS2_CMD_TEST_CONTROLLER);
    }

    // If the test still isn't returning the correct byte, abort.
    if (test_byte != PS2_CMD_RESPONSE_SELFTEST_PASS)
    {
        log("PS/2 controller self-test failed, aborting!\n");
        return;
    }

    // Test keyboard port.
    test_byte = ps2_send_cmd_response(PS2_CMD_TEST_KEYBPORT);
    for (uint8_t i = 0; i < 10; i++)
    {
        // Check and re-test if needed.
        if(test_byte == PS2_CMD_RESPONSE_PORTTEST_PASS)
            break;
        test_byte = ps2_send_cmd_response(PS2_CMD_TEST_KEYBPORT);
    }

    // If the test still isn't returning the correct byte, show error.
    if (test_byte != PS2_CMD_RESPONSE_PORTTEST_PASS)
        log("Keyboard PS/2 port self-test failed!\n");

    // Test mouse port.
    test_byte = ps2_send_cmd_response(PS2_CMD_TEST_MOUSEPORT);
    for (uint8_t i = 0; i < 10; i++)
    {
        // Check and re-test if needed.
        if(test_byte == PS2_CMD_RESPONSE_PORTTEST_PASS)
            break;
        test_byte = ps2_send_cmd_response(PS2_CMD_TEST_MOUSEPORT);
    }

    // If the test still isn't returning the correct byte, show error.
    if (test_byte != PS2_CMD_RESPONSE_PORTTEST_PASS)
        log("Mouse PS/2 port self-test failed!\n");

    // Ensure IRQs for the mouse and keyboard are disabled, but the ports are enabled.
    config &= ~(PS2_CONFIG_ENABLE_KEYBPORT_INTERRUPT | PS2_CONFIG_ENABLE_MOUSEPORT_INTERRUPT |
        PS2_CONFIG_DISABLE_KEYBPORT_CLOCK | PS2_CONFIG_DISABLE_MOUSEPORT_CLOCK);
    config |= PS2_CONFIG_ENABLE_KEYB_TRANSLATION;

    // Write config to controller.
    ps2_send_cmd(PS2_CMD_WRITE_BYTE);
    ps2_send_data(config);
    config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
    log("New PS/2 configuration byte: 0x");
    log(utoa(config, tmp, 16));
    log("\n");

    // Reset and test keyboard.
    test_byte = ps2_send_data_response(PS2_DATA_RESET);
    for (uint8_t i = 0; i < 10; i++)
    {
        // Check and re-test if needed.
        if(test_byte == PS2_DATA_RESPONSE_SELFTEST_PASS || test_byte == PS2_DATA_RESPONSE_ACK)
        
            break;
        test_byte = ps2_send_data_response(PS2_DATA_RESET);
    }

    // If the test still isn't returning the correct byte, show error.
    if(!(test_byte == PS2_DATA_RESPONSE_SELFTEST_PASS || test_byte == PS2_DATA_RESPONSE_ACK))
        log("Keyboard self-test failed!\n");

    // Reset and test mouse.
    ps2_send_cmd(PS2_CMD_WRITE_MOUSE_IN);
    test_byte = ps2_send_data_response(PS2_DATA_RESET);
    for (uint8_t i = 0; i < 10; i++)
    {
        // Check and re-test if needed.
        if(test_byte == PS2_DATA_RESPONSE_SELFTEST_PASS || test_byte == PS2_DATA_RESPONSE_ACK)
            break;
        ps2_send_cmd(PS2_CMD_WRITE_MOUSE_IN);
        test_byte = ps2_send_data_response(PS2_DATA_RESET);
    }

    // If the test still isn't returning the correct byte, show error.
    if(!(test_byte == PS2_DATA_RESPONSE_SELFTEST_PASS || test_byte == PS2_DATA_RESPONSE_ACK))
        log("Mouse self-test failed!\n");

    // Initialize devices.
    ps2_keyboard_init();
    ps2_mouse_init();

    // Read the current configuration byte.
    config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);

    log("Initial PS/2 configuration byte: 0x");
    log(utoa(config, tmp, 16));
    log("\n");

    // Enable interrupts for the mouse.
    config |= PS2_CONFIG_ENABLE_KEYBPORT_INTERRUPT | PS2_CONFIG_ENABLE_MOUSEPORT_INTERRUPT;

    // Write config to controller.
    ps2_send_cmd(PS2_CMD_WRITE_BYTE);
    ps2_send_data(config);
    config = ps2_send_cmd_response(PS2_CMD_READ_BYTE);
    log("New PS/2 configuration byte: 0x");
    log(utoa(config, tmp, 16));
    log("\n");

    log("PS/2 controller initialized!\n");
}
