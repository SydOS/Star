; 32-bit code.
[bits 32]

extern KERNEL_PHYSICAL_DATA_END
extern KERNEL_PHYSICAL_END

; Multiboot constants.
MULTIBOOT_HEADER_MAGIC  equ 0xe85250d6

; Multiboot section.
section .multiboot
multiboot_start:
	align 8

	; Magic value.
	dd MULTIBOOT_HEADER_MAGIC

	; Architecture. 0 = i386.
	dd 0

	; Header length.
	dd multiboot_end - multiboot_start

	; Header checksum.
	dd 0x100000000 - (MULTIBOOT_HEADER_MAGIC + 0 + (multiboot_end - multiboot_start))

	align 8

	; Address tag
	;dw 2
	;dw 0
	;dd 24
	;dd multiboot_start
	;dd multiboot_end
	;dd KERNEL_PHYSICAL_DATA_END
	;dd KERNEL_PHYSICAL_END

	;align 8

	; Entry address tag
	dw 3
	dw 0
	dd 12
	extern _start
	dd _start

	align 8

	; End tag.
	dw 0
	dw 0
	dd 8
multiboot_end:
