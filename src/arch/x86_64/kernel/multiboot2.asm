; 32-bit code.
[bits 32]

; Multiboot constants.
MULTIBOOT_HEADER_MAGIC  equ 0xe85250d6

; Multiboot section.
section .multiboot
multiboot_start:
align 8
	dd MULTIBOOT_HEADER_MAGIC
	dd 0
	dd multiboot_end - multiboot_start
	dd 0x100000000 - (MULTIBOOT_HEADER_MAGIC + 0 + (multiboot_end - multiboot_start))

	dw 0
	dw 0
	dd 8
multiboot_end:
