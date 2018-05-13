/*
 * File: rtc.c
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <main.h>
#include <io.h>
#include <kprint.h>
#include <tools.h>
#include <driver/rtc.h>
#include <kernel/memory/kheap.h>
#include <kernel/tasking.h>
#include <kernel/interrupts/irqs.h>

/**
 * Reads from a specific register for RTC
 * @param  reg RTC register to read from
 * @return     Output from RTC read register
 */
uint8_t rtc_read(uint16_t reg) {
	// (0 << 7) sets the NMI disable bit to false
	outb(0x70, (0 << 7) | reg);
	return inb(0x71);
}

/**
 * Converts BCD to human readable and comprehendable integers.
 * BCD is the time, written in hex. Like 0x32, 0x54, etc.
 */
void rtc_bcd_to_int(void) {
	rtc_time->seconds = (rtc_time->seconds & 0x0F) + ((rtc_time->seconds / 16) * 10);
    rtc_time->minutes = (rtc_time->minutes & 0x0F) + ((rtc_time->minutes / 16) * 10);
    rtc_time->hours = ( (rtc_time->hours & 0x0F) + (((rtc_time->hours & 0x70) / 16) * 10) ) | (rtc_time->hours & 0x80);
    rtc_time->day = (rtc_time->day & 0x0F) + ((rtc_time->day / 16) * 10);
    rtc_time->month = (rtc_time->month & 0x0F) + ((rtc_time->month / 16) * 10);
    rtc_time->year = (rtc_time->year & 0x0F) + ((rtc_time->year / 16) * 10);
}

/**
 * Gets the current time from the RTC
 */
void rtc_get_time() {
	rtc_time->seconds = rtc_read(0x00);
	rtc_time->minutes = rtc_read(0x02);
	rtc_time->hours = rtc_read(0x04);
	rtc_time->day = rtc_read(0x07);
	rtc_time->month = rtc_read(0x08);
	rtc_time->year = rtc_read(0x09);

	if(rtc_settings->binary_input == 0) {
		rtc_bcd_to_int();
	}
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
 * Thread to update the RTC every 500ms
 */
void rtc_thread() {
	while(true) {
		//rtc_get_time();
		sleep(500);
		//kprintf("%d:%d:%d %d/%d/%d\n", rtc_time->hours, rtc_time->minutes, rtc_time->seconds, rtc_time->month, rtc_time->day, rtc_time->year);
	}
}

/**
 * Init function for RTC
 */
void rtc_init() {
	struct RTCTime *time = (struct RTCTime*)kheap_alloc(sizeof(struct RTCTime));
	rtc_time = time;
	rtc_settings = rtc_get_settings();
	rtc_get_time();

	// Add poll thread.
	tasking_thread_schedule_proc(tasking_thread_create_kernel("rtc_worker", rtc_thread, 0, 0, 0), 0);
}