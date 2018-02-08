#include <main.h>
#include <driver/serial.h>
#include <driver/vga.h>

void log(const char* data) {
	for (size_t i = 0; i < strlen(data); i++) {
		serial_write(data[i]);
	}
	vga_writes(data);
}