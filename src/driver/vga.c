#include <main.h>
#include <tools.h>
#include <io.h>
#include <driver/vga.h>

/**
 * Returns a uint8_t for internal usage in coloring the terminal
 * @param  fg Foreground color from vga_color struct
 * @param  bg Background color from vga_color struct
 * @return    Internal usage uint8_t for directly writing to framebuffer
 */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

/**
 * Returns a uint16_t for internal usage in generating framebuffer entries
 * @param  uc    Unsigned char for character to write
 * @param  color Internal usage color from vga_entry_color()
 * @return       A uint16_t to write to the framebuffer to display a character
 */
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

/**
 * Current row position
 */
size_t terminal_row;
/**
 * Current column position
 */
size_t terminal_column;
/**
 * Current color, generated from vga_entry_color()
 */
uint8_t terminal_color;
/**
 * Pointer to RAM address where the framebuffer lives (0xB8000)
 */
uint16_t* terminal_buffer;
bool cursor_enabled = true;

/**
 * Clear out the framebuffer with empty chars, 
 * setting our color to light gray on black.
 */
void vga_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

/**
 * Set the framebuffer color for next write based on internally generated color
 * @param color An internally generated color from vga_entry_color
 */
void vga_internal_setcolor(uint8_t color) {
	terminal_color = color;
}

/**
 * Puts a character at a certain X and Y position
 * @param c     Character to write
 * @param color An internally generated color from vga_entry_color
 * @param x     X position in framebuffer
 * @param y     Y position in framebuffer
 */
void vga_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void vga_scroll() {
	terminal_row = VGA_HEIGHT-1;
	for (size_t y = 1; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index-VGA_WIDTH] = terminal_buffer[index];
		}
	}
	for (size_t i = terminal_column; i < VGA_WIDTH; i++) {
		vga_putentryat(' ', terminal_color, terminal_column, terminal_row);
		terminal_column++;
	}
	terminal_column = 0;
}

/**
 * Put a character to the framebuffer in the nearest available slot
 * @param c Character to write
 */
void vga_putchar(char c) {
	if (c == '\n') {
		for (size_t i = terminal_column; i < VGA_WIDTH; i++) {
			vga_putentryat(' ', terminal_color, terminal_column, terminal_row);
			terminal_column++;
		}
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			vga_scroll();
		}
		return;
	} else if(c == '\b' || c == 127) { // 127 = DEL control code.
		terminal_column--;
		return;
	} else if(c == '\r') {
		terminal_column = 0;
		return;
	} else {
		vga_putentryat(c, terminal_color, terminal_column, terminal_row);
	}

	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT) {
			vga_scroll();
		}
	}

	if (cursor_enabled == true) {
		vga_update_cursor(terminal_column, terminal_row);
	}
}

/**
 * Write a char array to the framebuffer
 * @param data Char array containing string to write
 * @param size Size of char array
 */
void vga_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++) {
		vga_putchar(data[i]);
	}

	if (cursor_enabled == true) {
		vga_update_cursor(terminal_column, terminal_row);
	}
}

/**
 * Publically accessable function to write char array to framebuffer
 * @param data Char array containing string to write
 */
void vga_writes(const char* data) {
	vga_write(data, strlen(data));
}

/**
 * Publically accessable function to the color of the terminal
 * @param fg Foreground color from vga_color struct
 * @param bg Background color from vga_color struct
 */
void vga_setcolor(enum vga_color fg, enum vga_color bg) {
	terminal_color = vga_entry_color(fg, bg);
}

void vga_enable_cursor() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 0);
 
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3E0) & 0xE0) | 15);

	cursor_enabled = true;
}

void vga_disable_cursor() {
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);

	cursor_enabled = false;
}

void vga_update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

int* vga_cursor_pos(int* p) {
	p[0] = terminal_column;
	p[1] = terminal_row;
	return p;
}