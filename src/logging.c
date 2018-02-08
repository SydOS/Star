#include <main.h>
#include <tools.h>
#include <driver/serial.h>
#include <driver/vga.h>

void log(const char* data) {
	for (size_t i = 0; i < strlen(data); i++) {
		if (data[i] == '\n') {
			serial_write('\n');
			serial_write('\r');
		} else {
			serial_write(data[i]);
		}
	}
	vga_writes(data);
}