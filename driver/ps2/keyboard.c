#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/keyboard.h>
#include <driver/ps2/ps2.h>
#include <kernel/interrupts.h>
#include <driver/vga.h>

// Keyboard commands.
enum
{
    PS2_KEYBOARD_CMD_SETLEDS    = 0xED,
    PS2_KEYBOARD_CMD_ECHO       = 0XEE,
    PS2_KEYBOARD_CMD_RESET      = 0xFF

};

// Keyboard responses.
enum
{
    PS2_KEYBOARD_RESPONSE_ERROR             = 0x00,
    PS2_KEYBOARD_RESPONSE_SELFTEST_PASS     = 0xAA,
    PS2_KEYBOARD_RESPONSE_ECHO              = 0xEE,
    PS2_KEYBOARD_RESPONSE_ACK               = 0xFA,
    PS2_KEYBOARD_RESPONSE_SELFTEST_FAIL     = 0xFC,
    PS2_KEYBOARD_RESPONSE_SELFTEST_FAIL2    = 0xFD,
    PS2_KEYBOARD_RESPONSE_SELFTEST_RESEND   = 0xFE,
    PS2_KEYBOARD_RESPONSE_ERROR2            = 0xFF
};

// Keyboard LEDs.
enum
{
    PS2_KEYBOARD_LED_NONE           = 0x00,
    PS2_KEYBOARD_LED_SCROLL_LOCK    = 0x01,
    PS2_KEYBOARD_LED_NUM_LOCK       = 0x02,
    PS2_KEYBOARD_LED_CAPS_LOCK      = 0x04
};

// Used for IRQ waiting.
static bool irq_triggered = false;

// United States keyboard layout.
const char keyboard_layout_us[] =
{
    0, 27, /* ESC */
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '-', '=',
    '\b', /* Backspace */
    '\t', /* Tab */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    'E', /* Enter */
    0, /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, /* Right shift */
    0, /* Print screen */
    0, /* Alt */
    ' ', /* Space bar */
    0 /* Caps lock */

};

void ps2_keyboard_wait_irq()
{
    uint8_t timeout = 300;
    while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(10);
	}
	if(!irq_triggered) { log("KEYBOARD IRQ TIMEOUT\n"); }
    irq_triggered = false;
}

void ps2_keyboard_send_cmd(uint8_t cmd)
{
    // Send command.
    outb(PS2_DATA_PORT, cmd);

    // Wait for IRQ.
    ps2_keyboard_wait_irq();
}

// Callback for keyboard on IRQ1.
static void ps2_keyboard_callback(registers_t* regs)
{	
	log("IRQ1 raised!\n");
    irq_triggered = true;


    // Read scancode from keyboard.
    
    uint8_t scancode = inb(PS2_DATA_PORT);
    if(scancode == 1)
        outb(0x64, 0xFE);

    if (scancode < sizeof(keyboard_layout_us))
    vga_putchar(keyboard_layout_us[scancode]);
}

// Initializes the keyboard.
void ps2_keyboard_init()
{
    // Register IRQ1 for the keyboard.
    interrupts_irq_install_handler(1, ps2_keyboard_callback);

    // Restore keyboard defaults and enable it.
    ps2_send_data_response(PS2_DATA_SET_DEFAULTS);
    ps2_send_data_response(PS2_DATA_ENABLE);
    log("PS/2 keyboard initialized!\n");
}
