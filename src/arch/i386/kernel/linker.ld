/*
 * File: linker.ld
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

/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(_start)
KERNEL_VIRTUAL_OFFSET = 0xC0000000;

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS {
	/* Kernel will be loaded at the 3GB + 1MB mark in virtual space, which
	   will be mapped to 1MB in the physical space. */

	. = 1M;
	KERNEL_INIT_START = .;
	.init ALIGN(4K) : {
		*(.multiboot);
		*(.init);
		*(.inittables);
	}

	KERNEL_INIT_END = .;
	. += KERNEL_VIRTUAL_OFFSET;

	/* For use within code. */
	KERNEL_VIRTUAL_START  = .;
    KERNEL_PHYSICAL_START = . - KERNEL_VIRTUAL_OFFSET;

	/* Multiboot header must be first, .text section, and read-only data. */
	.text ALIGN (0x1000) : AT(ADDR(.text) - 0xC0000000) {	
		*(.text)
		*(.rodata*)
	}

	/* Read-write data (initialized). */
	.data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000) {
		*(.data)
	}

	/* Read-write data (uninitialized) and stack. */
	.bss : AT(ADDR(.bss) - 0xC0000000) {
       *(COMMON)
       *(.bss)
	}

	/* For use within code. */
	KERNEL_VIRTUAL_END  = .;
    KERNEL_PHYSICAL_END = . - KERNEL_VIRTUAL_OFFSET;
}
