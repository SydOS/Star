#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/rtc.h>
#include <kernel/kheap.h>

uint8_t rtc_read(uint16_t reg) {
	outb(0x70, (0 << 7) | reg);
	return inb(0x71);
}

struct RTCTime* rtc_bcd_to_int(struct RTCTime* time) {
	time->seconds = (time->seconds & 0x0F) + ((time->seconds / 16) * 10);
    time->minutes = (time->minutes & 0x0F) + ((time->minutes / 16) * 10);
    time->hours = ( (time->hours & 0x0F) + (((time->hours & 0x70) / 16) * 10) ) | (time->hours & 0x80);
    time->day = (time->day & 0x0F) + ((time->day / 16) * 10);
    time->month = (time->month & 0x0F) + ((time->month / 16) * 10);
    time->year = (time->year & 0x0F) + ((time->year / 16) * 10);
    return time;
}

struct RTCTime* rtc_get_time() {
	struct RTCTime *time = (struct RTCTime*)kheap_alloc(sizeof(struct RTCTime));
	time->seconds = rtc_read(0x00);
	time->minutes = rtc_read(0x02);
	time->hours = rtc_read(0x04);
	time->day = rtc_read(0x07);
	time->month = rtc_read(0x08);
	time->year = rtc_read(0x09);

	struct RTC_Status_Register_B* settings = rtc_get_settings();
	if(settings->binary_input == 0) {
		rtc_bcd_to_int(time);
	}
	kheap_free(settings);

	return time;
}

struct RTC_Status_Register_B* rtc_get_settings() {
	struct RTC_Status_Register_B *settings = (struct RTC_Status_Register_B*)kheap_alloc(sizeof(struct RTC_Status_Register_B));
	uint8_t data = rtc_read(0x0B);
	settings->twentyfour_hour_time = (data & ( 1 << 1 )) >> 1;
	settings->binary_input = (data & ( 1 << 2 )) >> 2;
	return settings;
}