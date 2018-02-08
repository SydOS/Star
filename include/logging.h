#include <driver/serial.h>
#include <driver/vga.h>

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len]) { len++; }
	return len;
}

void log(const char* data) {
	for (size_t i = 0; i < strlen(data); i++) {
		serial_write(data);
	}
	vga_writes(data);
}