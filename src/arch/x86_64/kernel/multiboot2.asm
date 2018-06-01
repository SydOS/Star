;
; File: multiboot2.asm
; 
; Copyright (c) 2017-2018 Sydney Erickson, John Davis
; 
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;

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
