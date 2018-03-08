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

struct Status_Register_A {
	bool update_in_progess;
};

struct Status_Register_B {
	bool twentyfour_hour_time;
	bool binary_input;
};

extern struct RTCTime* rtc_get_time();