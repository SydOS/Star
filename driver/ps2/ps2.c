#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/ps2.h>

// Ports used for comms with controller.
#define PS2_DATA_PORT   0x60
#define PS2_REG_PORT    0x64

//void ps2_send_cmd()

void ps2_init()
{
    log("Intel 8042 PS/2 controller initialized!\n");
}
