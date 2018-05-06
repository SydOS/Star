#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

extern void ps2_keyboard_set_leds(bool numLock, bool capsLock, bool scrollLock);
//extern uint16_t ps2_keyboard_get_last_key(void);
extern void ps2_keyboard_init(void);

#endif
