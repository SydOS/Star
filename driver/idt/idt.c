#include <main.h>
#include <logging.h>
#include <driver/vga.h>

static uint32_t idt_location = 0x2000;
static uint32_t idtr_location = 0x10F0;
static uint32_t idt_size = 0x800;

static uint8_t __idt_setup = 0;
static uint8_t test_success = 0;
static uint32_t test_timeout = 0x1000;

/**
 * Set the Interrupt Descriptor Table Register (IDTR)
 */
extern void _set_idtr();
/**
 * Default (blank) handler for Interrupt Descriptor Table
 */
void __idt_default_handler();

#define INT_START asm volatile("pusha");
#define INT_END asm volatile("popa"); \
	asm volatile("iret");

/**
 * Register interrupt in IDT table at specific index
 * @param i        uint8_t representing index number
 * @param callback pointer to callback function
 */
void idt_register_interrupt(uint8_t i, uint32_t callback) {
	if(!__idt_setup) {
		vga_setcolor(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
		vga_writes("IDT not setup!\n");
		vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	}

	*(uint16_t*)(idt_location + 8*i + 0) = (uint16_t)(callback & 0x0000ffff);
	*(uint16_t*)(idt_location + 8*i + 2) = (uint16_t)0x8;
	*(uint8_t*) (idt_location + 8*i + 4) = 0x00;
	*(uint8_t*) (idt_location + 8*i + 5) = 0x8e;//0 | IDT_32BIT_INTERRUPT_GATE | IDT_PRESENT;
	*(uint16_t*)(idt_location + 8*i + 6) = (uint16_t)((callback & 0xffff0000) >> 16);
	return;
}

/**
 * Internal use stub for testing that IDT interrupts work
 */
void __idt_test_handler()
{
	INT_START;
	test_success = 1;
	INT_END;
}

/**
 * IDT table initialization code, should only be run once!
 */
void idt_init() {
	idt_location = 0x2000;
	idtr_location = 0x10F0;
	__idt_setup = 1;
	for(uint8_t i = 0; i < 255; i++)
	{
		idt_register_interrupt(i, (uint32_t)&__idt_default_handler);
	}
	idt_register_interrupt(0x2f, (uint32_t)&__idt_test_handler);
	//idt_register_interrupt(0x2e, (uint32_t)&schedule);
	/* create IDTR now */
	*(uint16_t*)idtr_location = idt_size - 1;
	*(uint32_t*)(idtr_location + 2) = idt_location;
	_set_idtr();
	asm volatile("int $0x2f");
	while(test_timeout-- != 0)
	{
		if(test_success != 0)
		{
			idt_register_interrupt(0x2F, (uint32_t)&__idt_default_handler);
			break;
		}
	}
	if(!test_success) {
		log("IDT link is offline (timeout).");
	}
	log("IDT Initialized!\n");
	return;
}