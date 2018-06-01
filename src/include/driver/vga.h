/*
 * File: vga.h
 * 
 * Copyright (c) 2017-2018 Sydney Erickson, John Davis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VGA_H
#define VGA_H

#include <main.h>

#define VGA_MEMORY_ADDRESS	0xB8000
#define VGA_WIDTH 	80
#define VGA_HEIGHT 	25

#define VGA_CRTC_ADDRESS	0x3D4
#define VGA_CRTC_DATA		0x3D5

#define VGA_REG_CURSOR_START	0x0A
#define VGA_REG_CURSOR_END		0x0B
#define VGA_REG_CURSOR_LOC_HIGH	0x0E
#define VGA_REG_CURSOR_LOC_LOW	0x0F

#define VGA_CURSOR_DISABLE			0x20
#define VGA_CURSOR_SCANLINE_START	0x0
#define VGA_CURSOR_SCANLINE_END		0xF

typedef enum {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
} vga_color_t;

extern void vga_update_cursor(uint16_t x, uint16_t y);
extern void vga_trigger_cursor_update(void);
extern void vga_enable_cursor(void);
extern void vga_disable_cursor(void);
extern int* vga_cursor_pos(int* p);

extern void vga_putentry(char c, uint16_t x, uint16_t y, vga_color_t fg, vga_color_t bg);
extern void vga_putchar(char c);
extern void vga_writes(const char* data);

extern void vga_setcolor(vga_color_t fg, vga_color_t bg);
extern void vga_setcolor_fg(vga_color_t fg);
extern void vga_setcolor_bg(vga_color_t bg);
extern void vga_init(void);

#endif
