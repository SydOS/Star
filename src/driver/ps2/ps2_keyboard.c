/*
 * File: ps2_keyboard.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <main.h>
#include <io.h>
#include <kprint.h>
#include <tools.h>
#include <driver/ps2/ps2_keyboard.h>
#include <libs/keyboard.h>

#include <driver/ps2/ps2.h>
#include <driver/vga.h>
#include <kernel/interrupts/irqs.h>
#include <kernel/memory/kheap.h>

// http://www.computer-engineering.org/ps2keyboard/scancodes2.html

#define PS2_KEYBOARD_SCANCODE_EXTENDED1     0xE0
#define PS2_KEYBOARD_SCANCODE_EXTENDED2     0xE1

// Translation table from set 1 to our internal keyset.
static const uint8_t ps2_scancodes[] =
{
    [0x00] = KEYBOARD_KEY_UNKNOWN,
    [0x01] = KEYBOARD_KEY_ESC,
    [0x02] = KEYBOARD_KEY_1,
    [0x03] = KEYBOARD_KEY_2,
    [0x04] = KEYBOARD_KEY_3,
    [0x05] = KEYBOARD_KEY_4,
    [0x06] = KEYBOARD_KEY_5,
    [0x07] = KEYBOARD_KEY_6,
    [0x08] = KEYBOARD_KEY_7,
    [0x09] = KEYBOARD_KEY_8,
    [0x0A] = KEYBOARD_KEY_9,
    [0x0B] = KEYBOARD_KEY_0,
    [0x0C] = KEYBOARD_KEY_HYPEN,
    [0x0D] = KEYBOARD_KEY_EQUALS,
    [0x0E] = KEYBOARD_KEY_BACKSPACE,
    [0x0F] = KEYBOARD_KEY_TAB,

    [0x10] = KEYBOARD_KEY_Q,
    [0x11] = KEYBOARD_KEY_W,
    [0x12] = KEYBOARD_KEY_E,
    [0x13] = KEYBOARD_KEY_R,
    [0x14] = KEYBOARD_KEY_T,
    [0x15] = KEYBOARD_KEY_Y,
    [0x16] = KEYBOARD_KEY_U,
    [0x17] = KEYBOARD_KEY_I,
    [0x18] = KEYBOARD_KEY_O,
    [0x19] = KEYBOARD_KEY_P,
    [0x1A] = KEYBOARD_KEY_OPEN_BRACKET,
    [0x1B] = KEYBOARD_KEY_CLOSE_BRACKET,
    [0x1C] = KEYBOARD_KEY_ENTER,
    [0x1D] = KEYBOARD_KEY_LEFT_CTRL,
    [0x1E] = KEYBOARD_KEY_A,
    [0x1F] = KEYBOARD_KEY_S,

    [0x20] = KEYBOARD_KEY_D,
    [0x21] = KEYBOARD_KEY_F,
    [0x22] = KEYBOARD_KEY_G,
    [0x23] = KEYBOARD_KEY_H,
    [0x24] = KEYBOARD_KEY_J,
    [0x25] = KEYBOARD_KEY_K,
    [0x26] = KEYBOARD_KEY_L,
    [0x27] = KEYBOARD_KEY_SEMICOLON,
    [0x28] = KEYBOARD_KEY_APOSTROPHE,
    [0x29] = KEYBOARD_KEY_GRAVE,
    [0x2A] = KEYBOARD_KEY_LEFT_SHIFT,
    [0x2B] = KEYBOARD_KEY_BACKSLASH,
    [0x2C] = KEYBOARD_KEY_Z,
    [0x2D] = KEYBOARD_KEY_X,
    [0x2E] = KEYBOARD_KEY_C,
    [0x2F] = KEYBOARD_KEY_V,

    [0x30] = KEYBOARD_KEY_B,
    [0x31] = KEYBOARD_KEY_N,
    [0x32] = KEYBOARD_KEY_M,
    [0x33] = KEYBOARD_KEY_COMMA,
    [0x34] = KEYBOARD_KEY_PERIOD,
    [0x35] = KEYBOARD_KEY_SLASH,
    [0x36] = KEYBOARD_KEY_RIGHT_SHIFT,
    [0x37] = KEYBOARD_KEY_NUM_STAR,
    [0x38] = KEYBOARD_KEY_LEFT_ALT,
    [0x39] = KEYBOARD_KEY_SPACE,
    [0x3A] = KEYBOARD_KEY_CAPS_LOCK,
    [0x3B] = KEYBOARD_KEY_F1,
    [0x3C] = KEYBOARD_KEY_F2,
    [0x3D] = KEYBOARD_KEY_F3,
    [0x3E] = KEYBOARD_KEY_F4,
    [0x3F] = KEYBOARD_KEY_F5,

    [0x40] = KEYBOARD_KEY_F6,
    [0x41] = KEYBOARD_KEY_F7,
    [0x42] = KEYBOARD_KEY_F8,
    [0x43] = KEYBOARD_KEY_F9,
    [0x44] = KEYBOARD_KEY_F10,
    [0x45] = KEYBOARD_KEY_NUM_LOCK,
    [0x46] = KEYBOARD_KEY_SCROLL_LOCK,
    [0x47] = KEYBOARD_KEY_NUM_7,
    [0x48] = KEYBOARD_KEY_NUM_8,
    [0x49] = KEYBOARD_KEY_NUM_9,
    [0x4A] = KEYBOARD_KEY_NUM_MINUS,
    [0x4B] = KEYBOARD_KEY_NUM_4,
    [0x4C] = KEYBOARD_KEY_NUM_5,
    [0x4D] = KEYBOARD_KEY_NUM_6,
    [0x4E] = KEYBOARD_KEY_NUM_PLUS,
    [0x4F] = KEYBOARD_KEY_NUM_1,

    [0x50] = KEYBOARD_KEY_NUM_2,
    [0x51] = KEYBOARD_KEY_NUM_3,
    [0x52] = KEYBOARD_KEY_NUM_0,
    [0x53] = KEYBOARD_KEY_NUM_PERIOD,
    [0x54] = KEYBOARD_KEY_SYSREQ,
    [0x55] = KEYBOARD_KEY_UNKNOWN,
    [0x56] = KEYBOARD_KEY_UNKNOWN,
    [0x57] = KEYBOARD_KEY_F11,
    [0x58] = KEYBOARD_KEY_F12
};

// Keyboard LEDs.
enum {
    PS2_KEYBOARD_LED_SCROLL_LOCK    = 0x01,
    PS2_KEYBOARD_LED_NUM_LOCK       = 0x02,
    PS2_KEYBOARD_LED_CAPS_LOCK      = 0x04
};

static uint16_t lastKeyCode = KEYBOARD_KEY_UNKNOWN;

static bool numLock = false;
static bool capsLock = false;
static bool scrollLock = false;

static bool ctrlPressed = false;
static bool shiftPressed = false;
static bool altPressed = false;

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

void ps2_keyboard_set_leds(bool num, bool caps, bool scroll) {
    // Update LEDs on keyboard.
    uint8_t data = (scroll ? PS2_KEYBOARD_LED_SCROLL_LOCK : 0) |
        (num ? PS2_KEYBOARD_LED_NUM_LOCK : 0) | (caps ? PS2_KEYBOARD_LED_CAPS_LOCK : 0);
    ps2_keyboard_send_cmd(PS2_DATA_SET_KEYBOARD_LEDS);
    ps2_keyboard_send_cmd(data);
  //  ps2_keyboard_send_cmd(PS2_DATA_SET_KEYBOARD_LEDS);
    //ps2_keyboard_send_cmd(data);
}

uint16_t ps2_keyboard_get_last_key(void *Driver) {
    uint16_t key = lastKeyCode;
    if (lastKeyCode != KEYBOARD_KEY_UNKNOWN)
        lastKeyCode = KEYBOARD_KEY_UNKNOWN;
    return key;
}

// Callback for keyboard on IRQ1.
static bool ps2_keyboard_callback(irq_regs_t *regs, uint8_t irqNum)
{	
    // Read data from keyboard.
    uint8_t data = ps2_get_data();
    static bool extended = false;

    // If the data is just an ACK, throw it out.
    if (data == PS2_DATA_RESPONSE_ACK)
        return true;

    // Do we have an extended code? If so, return and handle the actual code the next time around.
    if (data == PS2_KEYBOARD_SCANCODE_EXTENDED1 || data == PS2_KEYBOARD_SCANCODE_EXTENDED2) {
        extended = true;
        return true;
    }

    // Reset extended status.
    extended = false;

    // Get keycode.
    uint16_t key = ps2_scancodes[data & ~0x80];

    // Is the incoming code a make (press), or a break (release)?
    if (data & 0x80) { // Break code.
        // Handle special keys.
        switch (key) {
            case KEYBOARD_KEY_LEFT_CTRL:
            case KEYBOARD_KEY_RIGHT_CTRL:
                // Control key is released.
                ctrlPressed = false;
                break;

            case KEYBOARD_KEY_LEFT_SHIFT:
            case KEYBOARD_KEY_RIGHT_SHIFT:
                // Shift key is released.
                shiftPressed = false;
                break;

            case KEYBOARD_KEY_LEFT_ALT:
            case KEYBOARD_KEY_RIGHT_ALT:
                // Alt key is released.
                altPressed = false;
                break;
        }
    }
    else { // Make code.
        // Save key.
        lastKeyCode = key;

        // Handle special keys.
        switch (key) {
            case KEYBOARD_KEY_LEFT_CTRL:
            case KEYBOARD_KEY_RIGHT_CTRL:
                // Control key is pressed.
                ctrlPressed = true;
                break;

            case KEYBOARD_KEY_LEFT_SHIFT:
            case KEYBOARD_KEY_RIGHT_SHIFT:
                // Shift key is pressed.
                shiftPressed = true;
                break;

            case KEYBOARD_KEY_LEFT_ALT:
            case KEYBOARD_KEY_RIGHT_ALT:
                // Alt key is pressed.
                altPressed = true;
                break;

            case KEYBOARD_KEY_CAPS_LOCK:
                // Toggle caps lock.
                capsLock = !capsLock;
                ps2_keyboard_set_leds(numLock, capsLock, scrollLock);
                break;

            case KEYBOARD_KEY_SCROLL_LOCK:
                // Toggle scroll lock.
                scrollLock = !scrollLock;
                ps2_keyboard_set_leds(numLock, capsLock, scrollLock);
                break;

            case KEYBOARD_KEY_NUM_LOCK:
                // Toggle num lock.
                numLock = !numLock;
                ps2_keyboard_set_leds(numLock, capsLock, scrollLock);
                break;
        }
    }

    if (key == KEYBOARD_KEY_ESC && ctrlPressed && altPressed)
        ps2_reset_system();

  //  if(key == KEYBOARD_KEY_ESC)
     //   outb(0x64, 0xFE);

    /*if (key >= KEYBOARD_KEY_LEFT_CTRL)
        return;

    if ((shift_pressed && key <= KEYBOARD_KEY_SLASH) || (caps_lock && key <= KEYBOARD_KEY_Z) || (num_lock && key >= KEYBOARD_KEY_NUM_1 && key <= KEYBOARD_KEY_NUM_PERIOD))
        vga_putchar(keyboard_layout_us[key].shift_ascii);
    else
        vga_putchar(keyboard_layout_us[key].ascii);*/
    return true;
}

// Initializes the keyboard.
void ps2_keyboard_init(void) {
    kprintf("PS/2 KEYBOARD: Initializing!\n");

    // Register IRQ1 for the keyboard.
    irqs_install_handler(IRQ_KEYBOARD, ps2_keyboard_callback);

    // Restore keyboard defaults and enable it.
    ps2_send_data_response(PS2_DATA_SET_DEFAULTS);
    ps2_send_data_response(PS2_DATA_ENABLE);

    // Reset LEDs.
    numLock = capsLock = scrollLock = false;
    ps2_keyboard_set_leds(false, false, false);
    ps2_keyboard_set_leds(false, false, false);
    ps2_keyboard_set_leds(false, false, false);
    ps2_keyboard_set_leds(false, false, false);
    lastKeyCode = KEYBOARD_KEY_UNKNOWN;

    keyboard_t *keyboard = (keyboard_t*)kheap_alloc(sizeof(keyboard_t));
    keyboard->Name = "PS/2 keyboard";
    //keyboard->Driver = usbKeyboard;
    keyboard->GetLastKey = ps2_keyboard_get_last_key;
    keyboard_add(keyboard);
    kprintf("PS/2 KEYBOARD: Initialized!\n");
}
