#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/keyboard.h>
#include <kernel/interrupts.h>
#include <driver/vga.h>

// Port used for comms with PS/2.
#define PS2_DATA_PORT 0x60

// Keyboard commands.
enum
{
    KEYBOARD_CMD_SETLEDS    = 0xED,
    KEYBOARD_CMD_ECHO       = 0XEE,
    KEYBOARD_CMD_RESET      = 0xFF

};

// Keyboard responses.
enum
{
    KEYBOARD_RESPONSE_ERROR             = 0x00,
    KEYBOARD_RESPONSE_SELFTEST_PASS     = 0xAA,
    KEYBOARD_RESPONSE_ECHO              = 0xEE,
    KEYBOARD_RESPONSE_ACK               = 0xFA,
    KEYBOARD_RESPONSE_SELFTEST_FAIL     = 0xFC,
    KEYBOARD_RESPONSE_SELFTEST_FAIL2    = 0xFD,
    KEYBOARD_RESPONSE_SELFTEST_RESEND   = 0xFE,
    KEYBOARD_RESPONSE_ERROR2            = 0xFF
};

// Keyboard LEDs.
enum
{
    KEYBOARD_LED_NONE           = 0x00,
    KEYBOARD_LED_SCROLL_LOCK    = 0x01,
    KEYBOARD_LED_NUM_LOCK       = 0x02,
    KEYBOARD_LED_CAPS_LOCK      = 0x04
};

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

static void keyboard_wait_ack()
{
    while(inb(PS2_DATA_PORT) != KEYBOARD_RESPONSE_ACK);
}

static void keyboard_reset()
{

}

// Callback for PIT channel 0 on IRQ0.
static void keyboard_callback(registers_t* regs)
{	
	//log("Keyboard pressed!");

    // Read scancode from keyboard.
    
    uint8_t scancode = inb(PS2_DATA_PORT);
    if (scancode < sizeof(keyboard_layout_us))
    vga_putchar(keyboard_layout_us[scancode]);
}

// Initializes the keyboard.
void keyboard_init()
{


    // Send echo to keyboard.
    //outb(PS2_DATA_PORT, KEYBOARD_CMD_ECHO);
    //keyboard_wait_ack();

    outb(PS2_DATA_PORT, KEYBOARD_CMD_RESET);
    //keyboard_wait_ack();
    sleep(2000);
    outb(PS2_DATA_PORT, KEYBOARD_CMD_SETLEDS);
    keyboard_wait_ack();
    outb(PS2_DATA_PORT, KEYBOARD_LED_NUM_LOCK | KEYBOARD_LED_CAPS_LOCK | KEYBOARD_LED_SCROLL_LOCK);
    keyboard_wait_ack();
    sleep(2000);
    outb(PS2_DATA_PORT, KEYBOARD_CMD_SETLEDS);
    keyboard_wait_ack();
    outb(PS2_DATA_PORT, KEYBOARD_LED_NONE);
    keyboard_wait_ack();

    // Register IRQ1 for the keyboard.
    interrupts_irq_install_handler(1, keyboard_callback);
    log("Keyboard initialized!\n");
}
