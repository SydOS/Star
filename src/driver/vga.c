#include <main.h>
#include <io.h>
#include <string.h>
#include <kprint.h>
#include <driver/vga.h>
#include <driver/speaker.h>

extern uint32_t KERNEL_VIRTUAL_OFFSET;

/**
 * Current row, column, and color.
 */
static uint16_t terminalRow = 0;
static uint16_t terminalColumn = 0;
static uint8_t terminalColor = 0;

/**
 * Pointer to RAM address where the framebuffer lives (0xB8000)
 */
static uint16_t* terminalBuffer;

// -----------------------------------------------------------------------------
// Cursor stuff
// -----------------------------------------------------------------------------
static bool cursorEnabled = false;

/**
 * Updates the position of the VGA cursor
 * @param x X coordinate for cursor
 * @param y Y coordinate for cursor
 */
void vga_update_cursor(uint16_t x, uint16_t y) {
	// Calculate position.
	uint16_t pos = y * VGA_WIDTH + x;
 
	// Set cursor position.
	outb(VGA_CRTC_ADDRESS, VGA_REG_CURSOR_LOC_LOW);
	outb(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
	outb(VGA_CRTC_ADDRESS, VGA_REG_CURSOR_LOC_HIGH);
	outb(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * Trigger an update to the cursor. Simple call to vga_update_cursor
 * with arguments pre-filled
 */
void vga_trigger_cursor_update(void) {
	vga_update_cursor(terminalColumn, terminalRow);
}

/**
 * Send data over I/O port to enable the VGA cursor and update
 */
void vga_enable_cursor(void) {
	// Enable cursor and set start/end.
	outb(VGA_CRTC_ADDRESS, VGA_REG_CURSOR_START);
	outb(VGA_CRTC_DATA, VGA_CURSOR_SCANLINE_START);
 
	outb(VGA_CRTC_ADDRESS, VGA_REG_CURSOR_END);
	outb(VGA_CRTC_DATA, VGA_CURSOR_SCANLINE_END);
	cursorEnabled = true;

	// Update position.
	vga_trigger_cursor_update();
}

/**
 * Send data over I/O port to disable the VGA cursor
 */
void vga_disable_cursor(void) {
	// Disable cursor.
	outb(VGA_CRTC_ADDRESS, VGA_REG_CURSOR_START);
	outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & VGA_CURSOR_DISABLE));
	cursorEnabled = false;
}

/**
 * Get the current X and Y position of the cursor
 * @param p A 2 element array to write results to
 * @return A 2 element array, X and Y
 */
int* vga_cursor_pos(int* p) {
	p[0] = terminalColumn;
	p[1] = terminalRow;
	return p;
}

// -----------------------------------------------------------------------------
// Printing stuff
// -----------------------------------------------------------------------------
/**
 * Returns a uint16_t for internal usage in generating framebuffer entries
 * @param  uc    Unsigned char for character to write
 * @param  color Internal usage color from vga_entry_color()
 * @return       A uint16_t to write to the framebuffer to display a character
 */
static inline uint16_t vga_entry(char c, uint8_t color) {
	return (uint16_t)c | (uint16_t) color << 8;
}

/**
 * Internal function for placing characters.
 * @param c     Character to write.
 * @param x     X position in framebuffer.
 * @param y     Y position in framebuffer.
 */
static inline void vga_putentry_int(char c, uint16_t x, uint16_t y) {
	terminalBuffer[(y * VGA_WIDTH) + x] = vga_entry(c, terminalColor);
}

/**
 * Puts a character at a certain X and Y position.
 * @param c     Character to write.
 * @param x     X position in framebuffer.
 * @param y     Y position in framebuffer.
 * @param fg	Foreground color.
 * @param bg	Background color.
 */
void vga_putentry(char c, uint16_t x, uint16_t y, vga_color_t fg, vga_color_t bg) {
	terminalBuffer[(y * VGA_WIDTH) + x] = vga_entry(c, fg | bg << 4);
}

/**
 * Scrolls the terminal up a line.
 */
static void vga_scroll(void) {
	// Shift entire screen up a row.
	terminalRow = VGA_HEIGHT - 1;
	for (uint16_t y = 1; y < VGA_HEIGHT; y++) {
		for (uint16_t x = 0; x < VGA_WIDTH; x++) {
			const uint16_t index = y * VGA_WIDTH + x;
			terminalBuffer[index - VGA_WIDTH] = terminalBuffer[index];
		}
	}
	
	// Blank newly-created row and reset column position.
	for (uint16_t i = terminalColumn; i < VGA_WIDTH; i++) {
		vga_putentry_int(' ', terminalColumn, terminalRow);
		terminalColumn++;
	}
	terminalColumn = 0;
}

/**
 * Put a character to the framebuffer in the nearest available slot.
 * @param c Character to write.
 */
void vga_putchar(char c) {
	if (c == '\n') {
		// Move to new line.
		terminalColumn = 0;
		if (++terminalRow == VGA_HEIGHT)
			vga_scroll();
	} else if(c == '\b' || c == 127) { // 127 = DEL control code.
		// Move back a position.
		terminalColumn--;
	} else if(c == '\r') {
		// Reset column.
		terminalColumn = 0;
	} else if(c == '\a') {
		//speaker_play_tone(1000, 10);
		return;
	} else {
		// Place normal character.
		vga_putentry_int(c, terminalColumn, terminalRow);

		// Scroll screen if needed.
		if (++terminalColumn == VGA_WIDTH) {
			terminalColumn = 0;
			if (++terminalRow == VGA_HEIGHT)
				vga_scroll();
		}
	}

	// Update cursor position if enabled.
	//if (cursorEnabled)
	//	vga_trigger_cursor_update();
}

/**
 * Write string to display.
 * @param data	String to write.
 */
void vga_writes(const char* data) {
	for (uint32_t i = 0; i < strlen(data); i++)
		vga_putchar(data[i]);
}

/**
 * Set color of terminal.
 * @param fg	Foreground color.
 * @param bg	Background color.
 */
void vga_setcolor(vga_color_t fg, vga_color_t bg) {
	terminalColor = fg | bg << 4;
}

/**
 * Set foreground color of terminal.
 * @param fg	Foreground color.
 */
void vga_setcolor_fg(vga_color_t fg) {
	// Clear current foreground and set.
	terminalColor &= 0xF0;
	terminalColor |= fg;
}

/**
 * Set background color of terminal.
 * @param bg	Background color.
 */
void vga_setcolor_bg(vga_color_t bg) {
	// Clear current background and set.
	terminalColor &= 0x0F;
	terminalColor |= bg << 4;
}

/**
 * Initializes VGA.
 * Clear out the framebuffer with empty chars, 
 * setting our color to light gray on black.
 */
void vga_init(void) {
	// Reset color.
	vga_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	// Get pointer to video memory.
	terminalBuffer = (uint16_t*)(VGA_MEMORY_ADDRESS + (uintptr_t)&KERNEL_VIRTUAL_OFFSET);

	// Blank out display.
	terminalRow = terminalColumn = 0;
	for (uint16_t y = 0; y < VGA_HEIGHT; y++) {
		for (uint16_t x = 0; x < VGA_WIDTH; x++)
			vga_putentry_int(' ', x, y);
	}
	vga_enable_cursor();
	kprintf("\e[91mV\e[92mG\e[94mA\e[0m: Initialized!\n");
}
