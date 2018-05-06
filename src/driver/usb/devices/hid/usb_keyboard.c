/*
 * File: usb_keyboard.c
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
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/devices/hid/usb_keyboard.h>
#include <driver/usb/devices/hid/usb_hid.h>
#include <driver/usb/devices/usb_device.h>
#include <driver/usb/devices/usb_requests.h>
#include <driver/usb/devices/usb_descriptors.h>
#include <libs/keyboard.h>
#include <kernel/memory/kheap.h>
#include <kernel/tasking.h>

static const uint16_t UsbKeyboardScancodes[] = {
    [USB_KEYBOARD_KEY_A] = KEYBOARD_KEY_A,
    [USB_KEYBOARD_KEY_B] = KEYBOARD_KEY_B,
    [USB_KEYBOARD_KEY_C] = KEYBOARD_KEY_C,
    [USB_KEYBOARD_KEY_D] = KEYBOARD_KEY_D,
    [USB_KEYBOARD_KEY_E] = KEYBOARD_KEY_E,
    [USB_KEYBOARD_KEY_F] = KEYBOARD_KEY_F,
    [USB_KEYBOARD_KEY_G] = KEYBOARD_KEY_G,
    [USB_KEYBOARD_KEY_H] = KEYBOARD_KEY_H,
    [USB_KEYBOARD_KEY_I] = KEYBOARD_KEY_I,
    [USB_KEYBOARD_KEY_J] = KEYBOARD_KEY_J,
    [USB_KEYBOARD_KEY_K] = KEYBOARD_KEY_K,
    [USB_KEYBOARD_KEY_L] = KEYBOARD_KEY_L,
    [USB_KEYBOARD_KEY_M] = KEYBOARD_KEY_M,
    [USB_KEYBOARD_KEY_N] = KEYBOARD_KEY_N,
    [USB_KEYBOARD_KEY_O] = KEYBOARD_KEY_O,
    [USB_KEYBOARD_KEY_P] = KEYBOARD_KEY_P,
    [USB_KEYBOARD_KEY_Q] = KEYBOARD_KEY_Q,
    [USB_KEYBOARD_KEY_R] = KEYBOARD_KEY_R,
    [USB_KEYBOARD_KEY_S] = KEYBOARD_KEY_S,
    [USB_KEYBOARD_KEY_T] = KEYBOARD_KEY_T,
    [USB_KEYBOARD_KEY_U] = KEYBOARD_KEY_U,
    [USB_KEYBOARD_KEY_V] = KEYBOARD_KEY_V,
    [USB_KEYBOARD_KEY_W] = KEYBOARD_KEY_W,
    [USB_KEYBOARD_KEY_X] = KEYBOARD_KEY_X,
    [USB_KEYBOARD_KEY_Y] = KEYBOARD_KEY_Y,
    [USB_KEYBOARD_KEY_Z] = KEYBOARD_KEY_Z,

    [USB_KEYBOARD_KEY_ENTER] = KEYBOARD_KEY_ENTER,
    [USB_KEYBOARD_KEY_BACKSPACE] = KEYBOARD_KEY_BACKSPACE
};

bool usb_keyboard_update_leds(usb_keyboard_t *usbKeyboard) {
    // Create output report.
    usb_keyboard_output_report_t outputReport = { };
    outputReport.NumLockLed = usbKeyboard->NumLock;
    outputReport.CapsLockLed = usbKeyboard->CapsLock;
    outputReport.ScrollLockLed = usbKeyboard->ScrollLock;

    // Build transfer to set status.
    usb_control_transfer_t transfer = { };
    transfer.Inbound = false;
    transfer.Type = USB_REQUEST_TYPE_CLASS;
    transfer.Recipient = USB_REQUEST_REC_INTERFACE;
    transfer.Request = USB_HID_REQUEST_SET_REPORT;
    transfer.ValueLow = 0;
    transfer.ValueHigh = 0x02;
    transfer.Index = usbKeyboard->Interface->Number;

    // Send report to keyboard.
    usbKeyboard->Device->ControlTransfer(usbKeyboard->Device, usbKeyboard->Device->EndpointZero, transfer, &outputReport, sizeof(usb_keyboard_output_report_t));
}

bool running = false;
usb_keyboard_t *usbKeyboardTmp;
void usb_keyboard_boop(void) {
    usb_keyboard_t *usbKeyboard = usbKeyboardTmp;
    running = true;
    usb_keyboard_input_report_t report = {};
    while (true) {
        if (usb_uhci_device_interrupt_in_poll(usbKeyboard->Device, usbKeyboard->DataEndpoint, &report, sizeof(usb_keyboard_input_report_t))) {
            if (report.Keycode1 >= USB_KEYBOARD_KEY_A)
                usbKeyboard->LastKeyCode = UsbKeyboardScancodes[report.Keycode1];
            else 
                usbKeyboard->LastKeyCode = KEYBOARD_KEY_UNKNOWN;
            // Handle LEDs.
            switch (report.Keycode1) {
                case USB_KEYBOARD_KEY_NUM_LOCK:
                    usbKeyboard->NumLock = !usbKeyboard->NumLock;
                    usb_keyboard_update_leds(usbKeyboard);
                    break;

                case USB_KEYBOARD_KEY_CAPS_LOCK:
                    usbKeyboard->CapsLock = !usbKeyboard->CapsLock;
                    usb_keyboard_update_leds(usbKeyboard);
                    break;

                case USB_KEYBOARD_KEY_SCROLL_LOCK:
                    usbKeyboard->ScrollLock = !usbKeyboard->ScrollLock;
                    usb_keyboard_update_leds(usbKeyboard);
                    break;
            }

         /*   kprintf("USB KBD:");
            if (report.LeftControl)
                kprintf(" LCTRL");
            if (report.LeftShift)
                kprintf(" LSHIFT");
            if (report.LeftAlt)
                kprintf(" LALT");
            if (report.LeftGui)
                kprintf(" LGUI");
            if (report.RightControl)
                kprintf(" RCTRL");
            if (report.RightShift)
                kprintf(" RSHIFT");
            if (report.RightAlt)
                kprintf(" RALT");
            if (report.RightGui)
                kprintf(" RGUI");
            kprintf("\n");
            kprintf("USB KBD codes: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", report.Keycode1, report.Keycode2, report.Keycode3, report.Keycode4, report.Keycode5, report.Keycode6);*/
        }
    }
}

void usb_keyboard_print_info(usb_keyboard_t *usbKeyboard) {
    usb_device_t *usbDevice = usbKeyboard->Device;
    kprintf("USB KBD: USB HID keyboard on USB device %u, interface %u:\n", usbDevice->Address, usbKeyboard->Interface->Number);
    kprintf("USB KBD: Device: %s %s (%4X:%4X)\n", usbDevice->VendorString, usbDevice->ProductString, usbDevice->VendorId, usbDevice->ProductId);
    if (usbKeyboard->DataEndpoint != NULL)
        kprintf("USB KBD: Data-in endpoint #%u\n", usbKeyboard->DataEndpoint->Number);
    else
        kprintf("USB KBD: Data-in using control transfers\n");
    if (usbKeyboard->StatusOutEndpoint != NULL)
        kprintf("USB KBD: Status-out endpoint #%u\n", usbKeyboard->StatusOutEndpoint->Number);
    else
        kprintf("USB KBD: Status-out using control transfers\n");
}

uint16_t usb_keyboard_get_last_key(void *driver) {
    // Get USB keyboard.
    usb_keyboard_t *usbKeyboard = (usb_keyboard_t*)driver;
    uint16_t key = usbKeyboard->LastKeyCode;
    if (usbKeyboard->LastKeyCode != KEYBOARD_KEY_UNKNOWN)
        usbKeyboard->LastKeyCode = KEYBOARD_KEY_UNKNOWN;
    return key;
}

bool usb_keyboard_init(usb_device_t *usbDevice, usb_interface_t *interface, uint8_t *interfaceConfBuffer, uint8_t *interfaceConfBufferEnd) {
    // Ensure interface is a keyboard.
    if (interface->Class != USB_CLASS_HID || interface->Protocol != USB_HID_PROTOCOL_KEYBOARD)
        return false;

    // Configure device if needed.
    kprintf("USB KBD: Initializing HID keyboard %u...\n", usbDevice->Address);
    if (!usbDevice->Configured) {
        if (!usb_device_configure(usbDevice))
            return false;
        usbDevice->Configured = true;
    }

    // Create USB keyboard object.
    usb_keyboard_t *usbKeyboard = (usb_keyboard_t*)kheap_alloc(sizeof(usb_keyboard_t));
    memset(usbKeyboard, 0, sizeof(usb_keyboard_t));
    usbKeyboard->Device = usbDevice;
    usbKeyboard->Interface = interface;

    // Get data IN endpoint.
    for (uint8_t e = 0; e < interface->NumEndpoints; e++) {
        usb_endpoint_t *endpoint = interface->Endpoints[e];
        if (endpoint->Inbound && endpoint->Type == USB_ENDPOINT_TRANSFERTYPE_INTERRUPT) {
            usbKeyboard->DataEndpoint = endpoint;
            break;
        }
    }

    // Get data OUT endpoint.
    for (uint8_t e = 0; e < interface->NumEndpoints; e++) {
        usb_endpoint_t *endpoint = interface->Endpoints[e];
        if (!endpoint->Inbound && endpoint->Type == USB_ENDPOINT_TRANSFERTYPE_INTERRUPT) {
            usbKeyboard->StatusOutEndpoint = endpoint;
            break;
        }
    }

    // Print info.
    usb_keyboard_print_info(usbKeyboard);

    keyboard_t *keyboard = (keyboard_t*)kheap_alloc(sizeof(keyboard_t));
    keyboard->Name = usbDevice->ProductString;
    keyboard->Driver = usbKeyboard;
    keyboard->GetLastKey = usb_keyboard_get_last_key;
    keyboard_add(keyboard);

    usbKeyboardTmp = usbKeyboard;
    running = false;
    usbDevice->InterruptTransferStart(usbDevice, usbKeyboard->DataEndpoint, sizeof(usb_keyboard_input_report_t));
   // tasking_add_process(tasking_create_process("usbkd", usb_keyboard_boop, 0, 0));
    //while (!running);
    //running = false;

    return true;
}