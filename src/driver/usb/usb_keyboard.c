#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_keyboard.h>
#include <driver/usb/usb_hid.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

bool running = false;
usb_keyboard_t *usbKeyboardTmp;
void usb_keyboard_boop(void) {
    usb_keyboard_t *usbKeyboard = usbKeyboardTmp;
    running = true;
    usb_keyboard_input_report_t report = {};
    while (true) {
        
        if (usb_uhci_device_interrupt_in_poll(usbKeyboard->Device, usbKeyboard->DataEndpoint, &report, sizeof(usb_keyboard_input_report_t))) {
        kprintf("USB KBD:");
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
        kprintf("USB KBD codes: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", report.Keycode1, report.Keycode2, report.Keycode3, report.Keycode4, report.Keycode5, report.Keycode6);
        }
    }
}

bool usb_keyboard_init(usb_device_t *usbDevice, usb_interface_t *interface) {
    // Ensure interface is a keyboard.
    if (interface->Class != USB_CLASS_HID || interface->Subclass != USB_HID_SUBCLASS_BOOT
        || interface->Protocol != USB_HID_PROTOCOL_KEYBOARD)
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
    usbKeyboard->Device = usbDevice;
    for (uint8_t e = 0; e < interface->NumEndpoints; e++) {
        usb_endpoint_t *endpoint = interface->Endpoints[e];
        if (endpoint->Inbound && endpoint->Type == USB_ENDPOINT_TRANSFERTYPE_INTERRUPT) {
            usbKeyboard->DataEndpoint = endpoint;
            break;
        }
    }

    

    usbKeyboardTmp = usbKeyboard;
    running = false;
    usb_uhci_device_interrupt_in_start(usbDevice, usbKeyboard->DataEndpoint, sizeof(usb_keyboard_input_report_t));
    tasking_add_process(tasking_create_process("usbkd", usb_keyboard_boop, 0, 0));
    while (!running);
    running = false;

    return true;
}