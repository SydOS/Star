#include <main.h>
#include <io.h>
#include <driver/idt.h>
#include <driver/pic.h>
#include <driver/vga.h>

#define PIT_REG_COUNTER0 0x40
#define PIT_REG_COUNTER1 0x41
#define PIT_REG_COUNTER2 0x42
#define PIT_REG_COMMAND  0x43

#define PIT_OCW_MASK_BINCOUNT 1
#define PIT_OCW_MASK_MODE 0xE
#define PIT_OCW_MASK_RL 0x30
#define PIT_OCW_MASK_COUNTER 0xC0

#define PIT_OCW_BINCOUNT_BINARY 0
#define PIT_OCW_BINCOUNT_BCD 1

#define PIT_OCW_MODE_TERMINALCOUNT 0
#define PIT_OCW_MODE_ONESHOT 0x2
#define PIT_OCW_MODE_RATEGEN 0x4
#define PIT_OCW_MODE_SQUAREWAVEGEN 0x6
#define PIT_OCW_MODE_SOFTWARETRIG 0x8
#define PIT_OCW_MODE_HARDWARETRIG 0xA

#define PIT_OCW_RL_LATCH 0
#define PIT_OCW_RL_LSBONLY 0x10
#define PIT_OCW_RL_MSBONLY 0x20
#define PIT_OCW_RL_DATA 0x30

#define PIT_OCW_COUNTER_0 0
#define PIT_OCW_COUNTER_1 0x40
#define PIT_OCW_COUNTER_2 0x80

static uint8_t task = 0;

/**
 * Basic IRQ handler
 */
void pit_irq()
{
	if(!task) {
		asm volatile("add $0x1c, %esp");
		asm volatile("pusha");
		PIC_sendEOI(0);
		asm volatile("popa");
		asm volatile("iret");
	} else {
		//asm volatile("add $0x1c, %esp");
		//schedule();
	}
}

/**
 * Internal function to send command to PIT chip
 * @param cmd Command to send
 */
static inline void __pit_send_command(uint8_t cmd)
{
	outb(PIT_REG_COMMAND, cmd);
}

/**
 * Internal function to send data to PIT
 * @param data    2 bytes of data to send
 * @param counter PIT counter port to send to
 */
static inline void __pit_send_data(uint16_t data, uint8_t counter)
{
	uint8_t	port = (counter==PIT_OCW_COUNTER_0) ? PIT_REG_COUNTER0 :
		((counter==PIT_OCW_COUNTER_1) ? PIT_REG_COUNTER1 : PIT_REG_COUNTER2);

	outb(port, (uint8_t)data);
}

/**
 * Internal function to read data to PIT
 * @param  counter PIT counter port to read from
 * @return         A byte of data
 */
static inline uint8_t __pit_read_data (uint16_t counter) {

	uint8_t	port = (counter==PIT_OCW_COUNTER_0) ? PIT_REG_COUNTER0 :
		((counter==PIT_OCW_COUNTER_1) ? PIT_REG_COUNTER1 : PIT_REG_COUNTER2);

	return inb(port);
}

/**
 * Start a PIT timer
 * @param freq    Frequency for timer
 * @param counter PIT counter port
 * @param mode    PIT counter mode
 */
static void pit_start_counter(uint32_t freq, uint8_t counter, uint8_t mode) {
	if (freq == 0) {
		return;
	}

	uint16_t divisor = (uint16_t)(1193181/(uint16_t)freq);

	// Send command words
	uint8_t ocw = 0;
	ocw = (ocw & ~PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
	ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | counter;
	__pit_send_command(ocw);

	// Set frequency rate
	__pit_send_data(divisor & 0xff, 0);
	__pit_send_data((divisor >> 8) & 0xff, 0);
}

/**
 * Initialize the PIT with a counter used for multitasking
 */
void pit_init() {
	idt_register_interrupt(32, (uint32_t)pit_irq);
	pit_start_counter(200, PIT_OCW_COUNTER_0, PIT_OCW_MODE_SQUAREWAVEGEN);
	vga_writes("PIT initialized!\n");
}