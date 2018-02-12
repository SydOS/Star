#include <main.h>
#include <io.h>
#include <logging.h>
#include <tools.h>
#include <driver/ps2/mouse.h>
#include <driver/ps2/ps2.h>
#include <kernel/interrupts.h>

// Used for IRQ waiting.
static bool irq_triggered = false;

void ps2_mouse_wait_irq()
{
    uint8_t timeout = 300;
    while (!irq_triggered) {
		if(!timeout) break;
		timeout--;
		sleep(10);
	}
	if(!irq_triggered) { log("MOUSE IRQ TIMEOUT\n"); }
    irq_triggered = false;
}

void ps2_mouse_send_cmd(uint8_t cmd)
{
    ps2_mouse_wait(1);
    outb(0x64, 0xD4);
    // Send command.
    ps2_mouse_wait(1);
    outb(PS2_DATA_PORT, cmd);
    
    // Wait for IRQ.
    //ps2_mouse_wait_irq();
}

void ps2_mouse_wait(uint8_t type)
{
    uint32_t timeout = 1000000;
    if (type == 0)
    {
        while (timeout--)
            if((inb(0x64) & 1) == 1)
                return;
    }
    else
    {
        while (timeout--)
            if((inb(0x64) & 2) == 0)
                return;
    }
}

// Callback for mouse on IRQ12.
static void ps2_mouse_callback(registers_t* regs)
{	
	log("IRQ12 raised!\n");
    irq_triggered = false;
    inb(PS2_DATA_PORT);
    outb(PS2_DATA_PORT, 0xEB);
    inb(PS2_DATA_PORT);
}

// Initializes the mouse.
void ps2_mouse_init()
{
    // Test port.
    /*if (ps2_send_cmd_response(PS2_CMD_TEST_MOUSEPORT) != PS2_CMD_RESPONSE_PORTTEST_PASS)
    {
        log("Mouse PS/2 port self-test failed!\n");
        return;
    }*/



    // Enable mouse.
    log ("Enable mouse port...\n");
   // ps2_mouse_wait(1);
    ps2_send_cmd(PS2_CMD_ENABLE_MOUSEPORT);

    // Enable interrupts from mouse.
    //ps2_mouse_wait(1);
    //uint8_t config = ps2_read_configuration();
   // ps2_mouse_wait(1);
    //ps2_configure(config | 0x02);
    //config = ps2_read_configuration();

    ps2_mouse_wait(1);
    outb(0x64, 0x20);
    ps2_mouse_wait(0);
    uint8_t status = inb(0x60) | 2;
    ps2_mouse_wait(1);
    outb(0x64, 0x60);
    ps2_mouse_wait(1);
    outb(0x60, status);

    //char* tmp;
    //log(utoa(config, tmp, 2));


    log("Resetting mouse...\n");
    ps2_mouse_send_cmd(0xFF);
    ps2_mouse_wait(0);
    inb(0x60);
    //ps2_mouse_send_cmd(0xEA);
   // log(utoa(inb(PS2_DATA_PORT), tmp, 16));
    ps2_mouse_send_cmd(0xF6);
    ps2_mouse_wait(0);
    inb(0x60);
   // log(utoa(inb(PS2_DATA_PORT), tmp, 16));
    //log(utoa(inb(PS2_DATA_PORT), tmp, 16));
    //log(utoa(inb(PS2_DATA_PORT), tmp, 16));
    //log(utoa(inb(PS2_DATA_PORT), tmp, 16));

    ps2_mouse_send_cmd(0xF4);
    ps2_mouse_wait(0);
    inb(0x60);
  //  log(utoa(inb(PS2_DATA_PORT), tmp, 16));

      // Register IRQ12 for the mouse.
    interrupts_irq_install_handler(12, ps2_mouse_callback);

     // config = ps2_read_configuration();
   // ps2_mouse_wait(1);
    //ps2_configure(config | 0x02);
   // config = ps2_read_configuration();

   // tmp;
  //  log(utoa(config, tmp, 2));

    log("PS/2 mouse initialized!\n");
}
