struct RTCTime {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t weekday;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t century;
};

struct RTC_Status_Register_A {
	bool update_in_progess;
};

struct RTC_Status_Register_B {
	bool twentyfour_hour_time;
	bool binary_input;
};

extern void rtc_init();

struct RTCTime* rtc_time;
struct RTC_Status_Register_B* rtc_settings;