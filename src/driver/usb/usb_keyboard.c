#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_keyboard.h>
#include <driver/usb/usb_hid.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

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
        
        if (usb_uhci_device_interrupt_in_poll(usbKeyboard->Device, usbKeyboard->DataEndpoint, &report, sizeof(usb_keyboard_input_report_t)) && report.Keycode1 != 0) {
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

    usbKeyboardTmp = usbKeyboard;
    running = false;
    usb_uhci_device_interrupt_in_start(usbDevice, usbKeyboard->DataEndpoint, sizeof(usb_keyboard_input_report_t));
    tasking_add_process(tasking_create_process("usbkd", usb_keyboard_boop, 0, 0));
    while (!running);
    running = false;

    return true;
}