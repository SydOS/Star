#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <main.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_hid.h>

#define USB_KEYBOARD_NO_EVENT               0x00
#define USB_KEYBOARD_ERROR_ROLLOVER         0x01
#define USB_KEYBOARD_POST_FAIL              0x02
#define USB_KEYBOARD_ERROR_UNKNOWN          0x03
#define USB_KEYBOARD_KEY_A                  0x04
#define USB_KEYBOARD_KEY_B                  0x05
#define USB_KEYBOARD_KEY_C                  0x06
#define USB_KEYBOARD_KEY_D                  0x07
#define USB_KEYBOARD_KEY_E                  0x08
#define USB_KEYBOARD_KEY_F                  0x09
#define USB_KEYBOARD_KEY_G                  0x0A
#define USB_KEYBOARD_KEY_H                  0x0B
#define USB_KEYBOARD_KEY_I                  0x0C
#define USB_KEYBOARD_KEY_J                  0x0D
#define USB_KEYBOARD_KEY_K                  0x0E
#define USB_KEYBOARD_KEY_L                  0x0F
#define USB_KEYBOARD_KEY_M                  0x10
#define USB_KEYBOARD_KEY_N                  0x11
#define USB_KEYBOARD_KEY_O                  0x12
#define USB_KEYBOARD_KEY_P                  0x13
#define USB_KEYBOARD_KEY_Q                  0x14
#define USB_KEYBOARD_KEY_R                  0x15
#define USB_KEYBOARD_KEY_S                  0x16
#define USB_KEYBOARD_KEY_T                  0x17
#define USB_KEYBOARD_KEY_U                  0x18
#define USB_KEYBOARD_KEY_V                  0x19
#define USB_KEYBOARD_KEY_W                  0x1A
#define USB_KEYBOARD_KEY_X                  0x1B
#define USB_KEYBOARD_KEY_Y                  0x1C
#define USB_KEYBOARD_KEY_Z                  0x1D

#define USB_KEYBOARD_KEY_1                  0x1E
#define USB_KEYBOARD_KEY_2                  0x1F
#define USB_KEYBOARD_KEY_3                  0x20
#define USB_KEYBOARD_KEY_4                  0x21
#define USB_KEYBOARD_KEY_5                  0x22
#define USB_KEYBOARD_KEY_6                  0x23
#define USB_KEYBOARD_KEY_7                  0x24
#define USB_KEYBOARD_KEY_8                  0x25
#define USB_KEYBOARD_KEY_9                  0x26
#define USB_KEYBOARD_KEY_0                  0x27

#define USB_KEYBOARD_KEY_ENTER              0x28
#define USB_KEYBOARD_KEY_ESCAPE             0x29
#define USB_KEYBOARD_KEY_BACKSPACE          0x2A
#define USB_KEYBOARD_KEY_TAB                0x2B
#define USB_KEYBOARD_KEY_SPACE              0x2C
#define USB_KEYBOARD_KEY_HYPHEN             0x2D
#define USB_KEYBOARD_KEY_EQUALS             0x2E
#define USB_KEYBOARD_KEY_OPEN_BRACKET       0x2F
#define USB_KEYBOARD_KEY_CLOSE_BRACKET      0x30
#define USB_KEYBOARD_KEY_BACKSLASH          0x31
#define USB_KEYBOARD_KEY_POUND              0x32
#define USB_KEYBOARD_KEY_SEMICOLON          0x33
#define USB_KEYBOARD_KEY_QUOTE              0x34
#define USB_KEYBOARD_KEY_GRAVE_ACCENT       0x35
#define USB_KEYBOARD_KEY_COMMA              0x36
#define USB_KEYBOARD_KEY_PERIOD             0x37
#define USB_KEYBOARD_KEY_SLASH              0x38
#define USB_KEYBOARD_KEY_CAPS_LOCK          0x39

#define USB_KEYBOARD_KEY_F1                 0x3A
#define USB_KEYBOARD_KEY_F2                 0x3B
#define USB_KEYBOARD_KEY_F3                 0x3C
#define USB_KEYBOARD_KEY_F4                 0x3D
#define USB_KEYBOARD_KEY_F5                 0x3E
#define USB_KEYBOARD_KEY_F6                 0x3F
#define USB_KEYBOARD_KEY_F7                 0x40
#define USB_KEYBOARD_KEY_F8                 0x41
#define USB_KEYBOARD_KEY_F9                 0x42
#define USB_KEYBOARD_KEY_F10                0x43
#define USB_KEYBOARD_KEY_F11                0x44
#define USB_KEYBOARD_KEY_F12                0x45

#define USB_KEYBOARD_KEY_PRINT_SCREEN       0x46
#define USB_KEYBOARD_KEY_SCROLL_LOCK        0x47
#define USB_KEYBOARD_KEY_PAUSE              0x48
#define USB_KEYBOARD_KEY_INSERT             0x49
#define USB_KEYBOARD_KEY_HOME               0x4A
#define USB_KEYBOARD_KEY_PAGE_UP            0x4B
#define USB_KEYBOARD_KEY_DELETE             0x4C
#define USB_KEYBOARD_KEY_END                0x4D
#define USB_KEYBOARD_KEY_PAGE_DOWN          0x4E
#define USB_KEYBOARD_KEY_RIGHT              0x4F
#define USB_KEYBOARD_KEY_LEFT               0x50
#define USB_KEYBOARD_KEY_DOWN               0x51
#define USB_KEYBOARD_KEY_UP                 0x52
#define USB_KEYBOARD_KEY_NUM_LOCK           0x53

typedef struct {
    bool LeftControl : 1;
    bool LeftShift : 1;
    bool LeftAlt : 1;
    bool LeftGui : 1;
    bool RightControl : 1;
    bool RightShift : 1;
    bool RightAlt : 1;
    bool RightGui : 1;

    uint8_t Reserved;
    uint8_t Keycode1;
    uint8_t Keycode2;
    uint8_t Keycode3;
    uint8_t Keycode4;
    uint8_t Keycode5;
    uint8_t Keycode6;
} __attribute__((packed)) usb_keyboard_input_report_t;

typedef struct {
    bool NumLockLed : 1;
    bool CapsLockLed : 1;
    bool ScrollLockLed : 1;
    bool Compose : 1;
    bool Kana : 1;
    uint8_t Reserved : 3;
} __attribute__((packed)) usb_keyboard_output_report_t;

typedef struct {
    usb_device_t *Device;
    usb_interface_t *Interface;
    usb_endpoint_t *DataEndpoint;
    usb_endpoint_t *StatusOutEndpoint;


    usb_descriptor_hid_t *Descriptor;

    bool NumLock;
    bool CapsLock;
    bool ScrollLock;

    bool ShiftPressed;
    bool ControlPressed;
    bool AltPressed;
    bool GuiPressed;

    uint16_t LastKeyCode;
} usb_keyboard_t;

extern bool usb_keyboard_init(usb_device_t *usbDevice, usb_interface_t *interface, uint8_t *interfaceConfBuffer, uint8_t *interfaceConfBufferEnd);

#endif
