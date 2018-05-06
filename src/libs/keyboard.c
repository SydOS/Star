/*
 * File: keyboard.c
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
#include <kprint.h>
#include <string.h>
#include <libs/keyboard.h>

#include <driver/ps2/ps2_keyboard.h>
#include <driver/usb/devices/hid/usb_keyboard.h>

struct key_mapping
{
    const uint8_t key;
    const char ascii;
    const char shift_ascii;
};

// US QWERTY ASCII mappings.
const struct key_mapping keyboard_layout_us[] = {
    // Letters.
    [KEYBOARD_KEY_A] = { KEYBOARD_KEY_A, 'a', 'A' },
    [KEYBOARD_KEY_B] = { KEYBOARD_KEY_B, 'b', 'B' },
    [KEYBOARD_KEY_C] = { KEYBOARD_KEY_C, 'c', 'C' },
    [KEYBOARD_KEY_D] = { KEYBOARD_KEY_D, 'd', 'D' },
    [KEYBOARD_KEY_E] = { KEYBOARD_KEY_E, 'e', 'E' },
    [KEYBOARD_KEY_F] = { KEYBOARD_KEY_F, 'f', 'F' },
    [KEYBOARD_KEY_G] = { KEYBOARD_KEY_G, 'g', 'G' },
    [KEYBOARD_KEY_H] = { KEYBOARD_KEY_H, 'h', 'H' },
    [KEYBOARD_KEY_I] = { KEYBOARD_KEY_I, 'i', 'I' },
    [KEYBOARD_KEY_J] = { KEYBOARD_KEY_J, 'j', 'J' },
    [KEYBOARD_KEY_K] = { KEYBOARD_KEY_K, 'k', 'K' },
    [KEYBOARD_KEY_L] = { KEYBOARD_KEY_L, 'l', 'L' },
    [KEYBOARD_KEY_M] = { KEYBOARD_KEY_M, 'm', 'M' },
    [KEYBOARD_KEY_N] = { KEYBOARD_KEY_N, 'n', 'N' },
    [KEYBOARD_KEY_O] = { KEYBOARD_KEY_O, 'o', 'O' },
    [KEYBOARD_KEY_P] = { KEYBOARD_KEY_P, 'p', 'P' },
    [KEYBOARD_KEY_Q] = { KEYBOARD_KEY_Q, 'q', 'Q' },
    [KEYBOARD_KEY_R] = { KEYBOARD_KEY_R, 'r', 'R' },
    [KEYBOARD_KEY_S] = { KEYBOARD_KEY_S, 's', 'S' },
    [KEYBOARD_KEY_T] = { KEYBOARD_KEY_T, 't', 'T' },
    [KEYBOARD_KEY_U] = { KEYBOARD_KEY_U, 'u', 'U' },
    [KEYBOARD_KEY_V] = { KEYBOARD_KEY_V, 'v', 'V' },
    [KEYBOARD_KEY_W] = { KEYBOARD_KEY_W, 'w', 'W' },
    [KEYBOARD_KEY_X] = { KEYBOARD_KEY_X, 'x', 'X' },
    [KEYBOARD_KEY_Y] = { KEYBOARD_KEY_Y, 'y', 'Y' },
    [KEYBOARD_KEY_Z] = { KEYBOARD_KEY_Z, 'z', 'Z' },

    // Numbers.
    [KEYBOARD_KEY_1] = { KEYBOARD_KEY_1, '1', '!' },
    [KEYBOARD_KEY_2] = { KEYBOARD_KEY_2, '2', '@' },
    [KEYBOARD_KEY_3] = { KEYBOARD_KEY_3, '3', '#' },
    [KEYBOARD_KEY_4] = { KEYBOARD_KEY_4, '4', '$' },
    [KEYBOARD_KEY_5] = { KEYBOARD_KEY_5, '5', '%' },
    [KEYBOARD_KEY_6] = { KEYBOARD_KEY_6, '6', '^' },
    [KEYBOARD_KEY_7] = { KEYBOARD_KEY_7, '7', '&' },
    [KEYBOARD_KEY_8] = { KEYBOARD_KEY_8, '8', '*' },
    [KEYBOARD_KEY_9] = { KEYBOARD_KEY_9, '9', '(' },
    [KEYBOARD_KEY_0] = { KEYBOARD_KEY_0, '0', ')' },

    [KEYBOARD_KEY_ENTER] = { KEYBOARD_KEY_ENTER, '\n', '\n' },
    [KEYBOARD_KEY_ESC] = { KEYBOARD_KEY_ESC, '\e', '\e' },
    [KEYBOARD_KEY_BACKSPACE] = { KEYBOARD_KEY_BACKSPACE, '\b', '\b' },
    [KEYBOARD_KEY_TAB] = { KEYBOARD_KEY_TAB, '\t', '\t' },
    [KEYBOARD_KEY_SPACE] = { KEYBOARD_KEY_SPACE, ' ', ' ' },
    [KEYBOARD_KEY_GRAVE] = { KEYBOARD_KEY_GRAVE, '`', '~' },
    [KEYBOARD_KEY_HYPEN] = { KEYBOARD_KEY_HYPEN, '-', '_' },
    [KEYBOARD_KEY_EQUALS] = { KEYBOARD_KEY_EQUALS, '=', '+' },
    [KEYBOARD_KEY_OPEN_BRACKET] = { KEYBOARD_KEY_OPEN_BRACKET, '[', '{' },
    [KEYBOARD_KEY_CLOSE_BRACKET] = { KEYBOARD_KEY_CLOSE_BRACKET, ']', '}' },
    [KEYBOARD_KEY_BACKSLASH] = { KEYBOARD_KEY_BACKSLASH, '\\', '|' },
    [KEYBOARD_KEY_SEMICOLON] = { KEYBOARD_KEY_SEMICOLON, ';', ':' },
    [KEYBOARD_KEY_APOSTROPHE] = { KEYBOARD_KEY_APOSTROPHE, '\'', '"' },
    [KEYBOARD_KEY_COMMA] = { KEYBOARD_KEY_0, ',', '<' },
    [KEYBOARD_KEY_PERIOD] = { KEYBOARD_KEY_0, '.', '>' },
    [KEYBOARD_KEY_SLASH] = { KEYBOARD_KEY_0, '/', '?' },

    // Num pad.
    [KEYBOARD_KEY_NUM_SLASH] = { KEYBOARD_KEY_NUM_SLASH, '/', '/' },
    [KEYBOARD_KEY_NUM_STAR] = { KEYBOARD_KEY_NUM_STAR, '*', '*' },
    [KEYBOARD_KEY_NUM_MINUS] = { KEYBOARD_KEY_NUM_MINUS, '-', '-' },
    [KEYBOARD_KEY_NUM_PLUS] = { KEYBOARD_KEY_NUM_PLUS, '+', '+' },
    [KEYBOARD_KEY_NUM_ENTER] = { KEYBOARD_KEY_NUM_ENTER, '\r', '\r' },
    [KEYBOARD_KEY_NUM_1] = { KEYBOARD_KEY_NUM_1, 'E', '1' },
    [KEYBOARD_KEY_NUM_2] = { KEYBOARD_KEY_NUM_2, 'D', '2' },
    [KEYBOARD_KEY_NUM_3] = { KEYBOARD_KEY_NUM_3, 'P', '3' },
    [KEYBOARD_KEY_NUM_4] = { KEYBOARD_KEY_NUM_4, 'L', '4' },
    [KEYBOARD_KEY_NUM_5] = { KEYBOARD_KEY_NUM_5,  0, '5' },
    [KEYBOARD_KEY_NUM_6] = { KEYBOARD_KEY_NUM_6, 'R', '6' },
    [KEYBOARD_KEY_NUM_7] = { KEYBOARD_KEY_NUM_7, 'H', '7' },
    [KEYBOARD_KEY_NUM_8] = { KEYBOARD_KEY_NUM_8, 'U', '8' },
    [KEYBOARD_KEY_NUM_9] = { KEYBOARD_KEY_NUM_9, 'P', '9' },
    [KEYBOARD_KEY_NUM_0] = { KEYBOARD_KEY_NUM_0, 'I', '0' },
    [KEYBOARD_KEY_NUM_PERIOD] = { KEYBOARD_KEY_NUM_PERIOD, 'D', '.' },
};

keyboard_t *FirstKeyboard = NULL;

void keyboard_add(keyboard_t *keyboard) {
    keyboard->Next = FirstKeyboard;
    FirstKeyboard = keyboard;
    //keyboard->Next = FirstKeyboard->Next;
    //FirstKeyboard = keyboard;
}

void keyboard_remove(keyboard_t *keyboard) {

}

uint16_t keyboard_get_last_key(void) {
    keyboard_t *keyboard = FirstKeyboard;
    while (keyboard != NULL) {
        uint16_t key = keyboard->GetLastKey(keyboard->Driver);
        if (key != KEYBOARD_KEY_UNKNOWN)
            return key;
        keyboard = keyboard->Next;
    }
    return KEYBOARD_KEY_UNKNOWN;
}

char keyboard_get_ascii(uint16_t key) {
    if (key > KEYBOARD_KEY_NUM_PERIOD)
        return 0;
    return keyboard_layout_us[key].ascii;
}
