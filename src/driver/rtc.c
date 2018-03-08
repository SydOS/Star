#include <main.h>
#include <io.h>
#include <kprint.h>
#include <driver/rtc.h>

uint8_t rtc_read(uint16_t reg) {
	outb(0x70, (0 << 7) | reg);
	return inb(0x71);
}

struct RTCTime rtc_get_time() {
	struct RTCTime time;
	time.seconds = rtc_read(0x00);
	time.minutes = rtc_read(0x02);
	time.hours = rtc_read(0x04);
	time.day = rtc_read(0x07);
	time.month = rtc_read(0x08);
	time.year = rtc_read(0x09);
	kprintf("%d", time.seconds);
	return time;
}