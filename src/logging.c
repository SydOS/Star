#include <main.h>
#include <tools.h>
#include <driver/serial.h>
#include <driver/vga.h>

void log(const char* data) {
	serial_writes(data);
	vga_writes(data);
}