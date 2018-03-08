#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/rtc.h>
#include <kernel/kheap.h>
#include <kernel/tasking.h>
#include <arch/i386/kernel/interrupts.h>

void rtc_thread() {
	rtc_time = rtc_get_time();
	kprintf("%d:%d:%d %d/%d/%d\n", rtc_time->hours, rtc_time->minutes, 
		rtc_time->seconds, rtc_time->month, rtc_time->day, rtc_time->year);
	sleep(500);
}

/**
 * Reads from a specific register for RTC
 * @param  reg [description]
 * @return     [description]
 */
uint8_t rtc_read(uint16_t reg) {
	// (0 << 7) sets the NMI disable bit to false
	outb(0x70, (0 << 7) | reg);
	return inb(0x71);
}

/**
 * Converts BCD to human readable and comprehendable integers.
 * BCD is the time, written in hex. Like 0x32, 0x54, etc.
 * @param  time An RTCTime struct to read from
 */
void rtc_bcd_to_int(struct RTCTime* time) {
	time->seconds = (time->seconds & 0x0F) + ((time->seconds / 16) * 10);
    time->minutes = (time->minutes & 0x0F) + ((time->minutes / 16) * 10);
    time->hours = ( (time->hours & 0x0F) + (((time->hours & 0x70) / 16) * 10) ) | (time->hours & 0x80);
    time->day = (time->day & 0x0F) + ((time->day / 16) * 10);
    time->month = (time->month & 0x0F) + ((time->month / 16) * 10);
    time->year = (time->year & 0x0F) + ((time->year / 16) * 10);
}

/**
 * Gets the current time from the RTC
 * @return An RTCTime struct with all values filled
 */
struct RTCTime* rtc_get_time() {
	struct RTCTime *time = (struct RTCTime*)kheap_alloc(sizeof(struct RTCTime));
	time->seconds = rtc_read(0x00);
	time->minutes = rtc_read(0x02);
	time->hours = rtc_read(0x04);
	time->day = rtc_read(0x07);
	time->month = rtc_read(0x08);
	time->year = rtc_read(0x09);

	if(rtc_settings->binary_input == 0) {
		rtc_bcd_to_int(time);
	}

	return time;
}

/**
 * Gets the RTC related settings from the CMOS
 * @return Status_Register_B with RTC related settings filled
 */
struct RTC_Status_Register_B* rtc_get_settings() {
	struct RTC_Status_Register_B *settings = (struct RTC_Status_Register_B*)kheap_alloc(sizeof(struct RTC_Status_Register_B));
	uint8_t data = rtc_read(0x0B);

	settings->twentyfour_hour_time = (data & ( 1 << 1 )) >> 1;
	settings->binary_input = (data & ( 1 << 2 )) >> 2;

	return settings;
}

/**
 * Init function for RTC
 */
void rtc_init() {
	rtc_settings = rtc_get_settings();
	rtc_time = rtc_get_time();

	tasking_add_process(tasking_create_process("rtc", (uint32_t)rtc_thread));
}