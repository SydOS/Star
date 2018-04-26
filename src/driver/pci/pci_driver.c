#include <main.h>
#include <driver/pci.h>

#include <driver/usb/usb_uhci.h>
#include <driver/usb/usb_ohci.h>

// Array of PCI device drivers.
// Driver init() function must return a bool and accept a pci_device_t* as the only parameter.
const pci_driver_t PciDrivers[] = {

    // USB.
    { "UHCI host controller", usb_uhci_init },
    { "OHCI host controller", usb_ohci_init },

    // End driver.
    { "", NULL }
};
