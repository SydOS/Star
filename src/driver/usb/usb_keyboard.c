#include <main.h>
#include <tools.h>
#include <kprint.h>
#include <string.h>
#include <driver/usb/usb_keyboard.h>
#include <driver/usb/usb_hid.h>
#include <driver/usb/usb_device.h>
#include <driver/usb/usb_requests.h>
#include <driver/usb/usb_descriptors.h>

bool usb_keyboard_init(usb_device_t *usbDevice, usb_descriptor_interface_t *interfaceDesc) {
    // Ensure interface is a keyboard.
    if (interfaceDesc->InterfaceClass != USB_CLASS_HID || interfaceDesc->InterfaceSubclass != USB_HID_SUBCLASS_BOOT
        || interfaceDesc->InterfaceProtocol != USB_HID_PROTOCOL_KEYBOARD)
        return false;

    // Configure device if needed.
    kprintf("USB KBD: Initializing HID keyboard %u...\n", usbDevice->Address);
    if (!usbDevice->Configured) {
        if (!usb_device_configure(usbDevice))
            return false;
        usbDevice->Configured = true;
    }

    // Get the HID descriptor.
    uint8_t *currentConfBuffer = (uint8_t*)interfaceDesc + interfaceDesc->Length;
    usb_descriptor_hid_t *hidDesc = NULL;
    while(true) {
        // Get length and type.
        uint8_t length = currentConfBuffer[0];
        uint8_t type = currentConfBuffer[1];

        if (type == USB_DESCRIPTOR_TYPE_HID) {
            hidDesc = (usb_descriptor_hid_t*)currentConfBuffer;
            break;
        }

        currentConfBuffer += length;
    }

    // Get the HID descriptor.
    currentConfBuffer = (uint8_t*)interfaceDesc + interfaceDesc->Length;
    usb_descriptor_endpoint_t *endp = NULL;
    while(true) {
        // Get length and type.
        uint8_t length = currentConfBuffer[0];
        uint8_t type = currentConfBuffer[1];

        if (type == USB_DESCRIPTOR_TYPE_ENDPOINT) {
            endp = (usb_descriptor_endpoint_t*)currentConfBuffer;
            break;
        }

        currentConfBuffer += length;
    }

    // Create USB keyboard object.
    usb_keyboard_t *usbKeyboard = (usb_keyboard_t*)kheap_alloc(sizeof(usb_keyboard_t));
    usbKeyboard->Device = usbDevice;
    usbKeyboard->Descriptor = hidDesc;
    usbKeyboard->InputEndpointAddress = 1;

    usb_keyboard_input_report_t report = {};
    usb_endpoint_t d = { };
    d.Number = 1;
    
    usb_uhci_device_interrupt_in_start(usbDevice, &d, sizeof(usb_keyboard_input_report_t));
    while (true) {
        
        if (usb_uhci_device_interrupt_in_poll(usbDevice, &d, &report, sizeof(usb_keyboard_input_report_t))) {
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
    return true;
}