#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/keyboard.h>
#include <driver/ps2/ps2.h>
#include <kernel/interrupts.h>
#include <driver/vga.h>

// Keyboard LEDs.
enum
{
    PS2_KEYBOARD_LED_SCROLL_LOCK    = 0x01,
    PS2_KEYBOARD_LED_NUM_LOCK       = 0x02,
    PS2_KEYBOARD_LED_CAPS_LOCK      = 0x04
};

// Special keyboard scancodes.
enum
{
    PS2_KEYBOARD_SCANCODE_CAPSLOCK      = 0x3A,
    PS2_KEYBOARD_SCANCODE_NUMLOCK       = 0x45,
    PS2_KEYBOARD_SCANCODE_SCROLLLOCK    = 0x46
};

static bool num_lock = false;
static bool caps_lock = false;
static bool scroll_lock = false;

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

// Sends a command to the mouse.
uint8_t ps2_keyboard_send_cmd(uint8_t cmd)
{
    // Send command to keyboard. 
    uint8_t response = 0;
    for (uint8_t i = 0; i < 10; i++)
    {
        response = ps2_send_data_response(cmd);

        // If the response is valid, return it.
        if (response != PS2_DATA_RESPONSE_RESEND)
            break;
    }

    // Return response.
    return response;
}

static void ps2_keyboard_set_leds()
{
    // Update LEDs on keyboard.
    ps2_keyboard_send_cmd(PS2_DATA_SET_KEYBOARD_LEDS);
    ps2_keyboard_send_cmd((scroll_lock ? PS2_KEYBOARD_LED_SCROLL_LOCK : 0) |
        (num_lock ? PS2_KEYBOARD_LED_NUM_LOCK : 0) | (caps_lock ? PS2_KEYBOARD_LED_CAPS_LOCK : 0));
    ps2_keyboard_send_cmd(PS2_DATA_SET_KEYBOARD_LEDS);
    ps2_keyboard_send_cmd((scroll_lock ? PS2_KEYBOARD_LED_SCROLL_LOCK : 0) |
        (num_lock ? PS2_KEYBOARD_LED_NUM_LOCK : 0) | (caps_lock ? PS2_KEYBOARD_LED_CAPS_LOCK : 0));
}

// Callback for keyboard on IRQ1.
static void ps2_keyboard_callback(registers_t* regs)
{	
	//log("IRQ1 raised!\n");

    // Read scancode from keyboard.
    uint8_t scancode = ps2_get_data();

    if (scancode == PS2_KEYBOARD_SCANCODE_CAPSLOCK)
    {
        // Toggle caps lock.
        caps_lock = !caps_lock;
        ps2_keyboard_set_leds();
    }
    else if (scancode == PS2_KEYBOARD_SCANCODE_SCROLLLOCK)
    {
        // Toggle scroll lock.
        scroll_lock = !scroll_lock;
        ps2_keyboard_set_leds();
    }
    else if (scancode == PS2_KEYBOARD_SCANCODE_NUMLOCK)
    {
        // Toggle num lock.
        num_lock = !num_lock;
        ps2_keyboard_set_leds();
    }

    if(scancode == 1)
        outb(0x64, 0xFE);
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
