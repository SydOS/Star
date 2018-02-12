#include <main.h>
#include <io.h>
#include <tools.h>
#include <logging.h>
#include <driver/interrupts.h>

#define PIT_REG_COUNTER0 0x40
#define PIT_REG_COUNTER1 0x41
#define PIT_REG_COUNTER2 0x42
#define PIT_REG_COMMAND  0x43

#define PIT_OCW_MASK_BINCOUNT 1 //00000001
#define PIT_OCW_MASK_MODE 0xE //00001110
#define PIT_OCW_MASK_RL 0x30 //00110000
#define PIT_OCW_MASK_COUNTER 0xC0 //11000000

#define PIT_OCW_BINCOUNT_BINARY 0 //0
#define PIT_OCW_BINCOUNT_BCD 1 //1

#define PIT_OCW_MODE_TERMINALCOUNT 0 //0000
#define PIT_OCW_MODE_ONESHOT 0x2 //0010
#define PIT_OCW_MODE_RATEGEN 0x4 //0100
#define PIT_OCW_MODE_SQUAREWAVEGEN 0x6 //0110
#define PIT_OCW_MODE_SOFTWARETRIG 0x8 //1000
#define PIT_OCW_MODE_HARDWARETRIG 0xA //1010

#define PIT_OCW_RL_LATCH 0 //000000
#define PIT_OCW_RL_LSBONLY 0x10 //010000
#define PIT_OCW_RL_MSBONLY 0x20 //100000
#define PIT_OCW_RL_DATA 0x30 //110000

#define PIT_OCW_COUNTER_0 0 //00000000
#define PIT_OCW_COUNTER_1 0x40 //01000000
#define PIT_OCW_COUNTER_2 0x80 //10000000

static uint8_t task = 0;


/**
 * Internal function to send command to PIT chip
 * @param cmd Command to send
 */
static inline void __pit_send_cmd(uint8_t cmd)
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
static void pit_start_counter (uint32_t freq, uint8_t counter, uint8_t mode) {

	if (freq==0){
		return;
	}
	uint16_t divisor = (uint16_t)( 1193181 / (uint16_t)freq);

	// send operational command words
	uint8_t ocw = 0;
	ocw = (ocw & ~PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
	ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | counter;
	__pit_send_cmd (ocw);

	// set frequency rate
	__pit_send_data(divisor & 0xff, 0);
	__pit_send_data((divisor >> 8) & 0xff, 0);
}


uint32_t tick = 0;
uint32_t seconds = 0;
static void timer_callback(registers_t* regs)
{	
	tick++;
	if (tick % 18 == 0)
	{
		seconds++;
		char* temp;
		utoa(seconds, temp, 10);
		log(temp);
		log(" seconds have passed! (");
		utoa(tick, temp, 10);
		log(temp);
		log(" ticks)\n");
	}
}


// Initialize the PIT.
void pit_init() {
	// Firstly, register our timer callback.
    interrupts_irq_install_handler(0, timer_callback);
    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency. Important to note is
    // that the divisor must be small enough to fit into 16-bits.
    //uint32_t divisor = 1193180 / 200;

    // Send the command byte.
   // outb(0x43, 0x36);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
   // uint8_t l = (uint8_t)(divisor & 0xFF);
   // uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
  //  outb(0x40, l);
  //  outb(0x40, h);

	//pit_start_counter(200,PIT_OCW_COUNTER_0, PIT_OCW_MODE_SQUAREWAVEGEN);
	log("PIT initialized!\n");
}