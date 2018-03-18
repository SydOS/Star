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
