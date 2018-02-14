// https://wiki.osdev.org/%228042%22_PS/2_Controller

enum
{
    PS2_STATUS_OUTPUTBUFFERFULL = 0x01, // Output buffer status (0 = empty, 1 = full).
    PS2_STATUS_INPUTBUFFERFULL  = 0x02, // Input buffer status (0 = empty, 1 = full).
    PS2_STATUS_SYSTEM_FLAG      = 0x04, // Meant to be cleared on reset and set by firmware. 
    PS2_STATUS_CONTROLLERCMD    = 0x08, // Command/data (0 = data written to input buffer is data for PS/2 device, 1 = data written to input buffer is data for PS/2 controller command).
    PS2_STATUS_RESERVED         = 0x10, // Unknown (chipset specific).
    PS2_STATUS_MOUSEFULL        = 0x20, // Mouse buffer full.
    PS2_STATUS_TIMEOUTERROR     = 0x40, // Time-out error (0 = no error, 1 = time-out error).
    PS2_STATUS_PARITYERROR      = 0x80  // Parity error (0 = no error, 1 = parity error).
};

// PS/2 commands.
enum
{
    PS2_CMD_READ_BYTE               = 0x20, // Read "byte 0" from internal RAM.
    PS2_CMD_WRITE_BYTE              = 0x60, // Write next byte to "byte 0" of internal RAM.
    PS2_CMD_DISABLE_MOUSEPORT       = 0xA7, // Disable mouse PS/2 port (only if 2 PS/2 ports supported).
    PS2_CMD_ENABLE_MOUSEPORT        = 0xA8, // Enable mouse PS/2 port (only if 2 PS/2 ports supported).
    PS2_CMD_TEST_MOUSEPORT          = 0xA9, // Test mouse PS/2 port (only if 2 PS/2 ports supported).
    PS2_CMD_TEST_CONTROLLER         = 0xAA, // Test PS/2 Controller. Response: 0x55 test passed; 0xFC test failed.
    PS2_CMD_TEST_KEYBPORT           = 0xAB, // Test keyboard PS/2 port. Response: 0x00 test passed; 0x01, 0x02, 0x03, 0x04 = error.
    PS2_CMD_DISABLE_KEYBPORT        = 0xAD, // Disable keyboard PS/2 port.
    PS2_CMD_ENABLE_KEYBPORT         = 0xAE, // Enable keyboard PS/2 port.
    PS2_CMD_READ_DIAG               = 0xAC, // Diagnostic dump (real all bytes of internal RAM).
    PS2_CMD_READ_INPUT              = 0xC0, // Read controller input port.
    PS2_CMD_READ_OUTPUT             = 0xD0, // Read controller output port.
    PS2_CMD_WRITE_OUTPUT            = 0xD1, // Write next byte to Controller Output Port.
    PS2_CMD_WRITE_KEYBOARD_OUT      = 0xD2, // Write next byte to first PS/2 port output buffer (makes it look like the byte written was received from the first PS/2 port).
    PS2_CMD_WRITE_MOUSE_OUT         = 0xD3, // Write next byte to second PS/2 port output buffer (makes it look like the byte written was received from the second PS/2 port).
    PS2_CMD_WRITE_MOUSE_IN          = 0xD4  // Write next byte to second PS/2 port input buffer (sends next byte to the second PS/2 port).
};

// PS/2 command responses.
enum
{
    PS2_CMD_RESPONSE_PORTTEST_PASS              = 0x00,
    PS2_CMD_RESPONSE_PORTTEST_CLOCKSTUCKLOW     = 0x01,
    PS2_CMD_RESPONSE_PORTTEST_CLOCKSTUCKHIGH    = 0x02,
    PS2_CMD_RESPONSE_PORTTEST_DATASTUCKLOW      = 0x03,
    PS2_CMD_RESPONSE_PORTTEST_DATASTUCKHIGH     = 0x04,

    PS2_CMD_RESPONSE_SELFTEST_PASS              = 0x55,
    PS2_CMD_RESPONSE_SELFTEST_FAIL              = 0xFC,
};

// PS/2 configuration.
enum
{
    PS2_CONFIG_ENABLE_KEYBPORT_INTERRUPT        = 0x01, // First PS/2 port interrupt (1 = enabled, 0 = disabled).
    PS2_CONFIG_ENABLE_MOUSEPORT_INTERRUPT       = 0x02, // Second PS/2 port interrupt (1 = enabled, 0 = disabled, only if 2 PS/2 ports supported).
    PS2_CONFIG_SYSTEM_FLAG                      = 0x04, // System Flag (1 = system passed POST, 0 = your OS shouldn't be running).
    PS2_CONFIG_DISABLE_KEYBPORT_CLOCK           = 0x10, // First PS/2 port clock (1 = disabled, 0 = enabled).
    PS2_CONFIG_DISABLE_MOUSEPORT_CLOCK          = 0x20, // Second PS/2 port clock (1 = disabled, 0 = enabled, only if 2 PS/2 ports supported).
    PS2_CONFIG_ENABLE_KEYB_TRANSLATION          = 0x40  // First PS/2 port translation (1 = enabled, 0 = disabled).
};

// PS/2 output port values.
enum
{
    PS2_OUTPUT_IN_KEYB_FULL     = 0x10,
    PS2_OUTPUT_IN_MOUSE_FULL    = 0x20
};

// PS/2 device (data port) commands.
enum
{
    PS2_DATA_ENABLE             = 0xF4, // Enable device.
    PS2_DATA_SET_DEFAULTS       = 0xF6, // Set default parameters.
    PS2_DATA_RESET              = 0xFF  // Reset and start self-test.
};

enum
{
    PS2_DATA_RESPONSE_SELFTEST_PASS     = 0xAA,
    PS2_DATA_RESPONSE_ACK               = 0xFA,
    PS2_DATA_RESPONSE_RESEND            = 0xFE
};

// Port used for comms with PS/2.
#define PS2_DATA_PORT   0x60
#define PS2_CMD_PORT    0x64

extern void ps2_wait_send();
extern void ps2_wait_receive();
extern void ps2_send_cmd(uint8_t cmd);
extern uint8_t ps2_send_cmd_response(uint8_t cmd);
extern void ps2_send_data(uint8_t data);
extern uint8_t ps2_send_data_response(uint8_t data);
extern uint8_t ps2_get_data();
extern uint8_t ps2_get_status();
extern void ps2_flush();
extern void ps2_init();
