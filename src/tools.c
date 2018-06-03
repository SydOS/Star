/*
 * File: tools.c
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

#include <main.h>
#include <string.h>
#include <kernel/timer.h>

/**
 * Convert int to char array
 * @param  value  Input integer
 * @param  result Output buffer
 * @param  base   Number base (ex: base 10)
 * @return        Char array of number
 */
char* itoa(int32_t value, char* result, int base) {
		// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

		// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char* utoa(uint32_t value, char* result, int base) {
		// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

		// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

int32_t atoi(const char *str) {
	// Convert decimal string to number.
	int32_t value = 0;
	for (uint16_t i = 0; i < strlen(str); i++)
		value = value * 10 + (str[i] - '0');
	return value;
}

uint32_t random_seed = 1;

uint32_t rand() {
	random_seed = random_seed * 1103515245 + 12345;
	return (uint32_t)(random_seed / 63336) % 4294967295;
}

uint32_t maxrand(uint32_t seed, uint32_t max) {
	random_seed = random_seed + seed * 1103515245 + 12345;
	return (uint32_t)(random_seed / 65536) % (max+1);
}

// -----------------------------------------------------------------------------



// Sleep for the specified number of milliseconds.
void sleep(uint32_t ms)
{
	// 1 tick = 1 ms.
	uint64_t startTick = timer_ticks();
	uint64_t endTick = startTick + ms;
	uint32_t tick = timer_ticks();
	while (tick < endTick)
		tick = timer_ticks();
}