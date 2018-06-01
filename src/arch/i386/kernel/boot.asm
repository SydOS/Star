;
; File: boot.asm
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

; Multiboot constants.
MULTIBOOT_PAGE_ALIGN    equ 0x00000001
MULTIBOOT_MEMORY_INFO   equ 0x00000002
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

; Multiboot section.
section .multiboot
align 4
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM

; Allocate stack
section .bss
	align 16
stack_bottom:
	resb 16384
stack_top:

section .inittables
global MULTIBOOT_MAGIC
MULTIBOOT_MAGIC: dd 0
global MULTIBOOT_INFO
MULTIBOOT_INFO: dd 0

; Start function.
section .init
global _start
_start:
	; Disable interrupts.
	cli

	; Print `SYDOS` to screen corner.
    mov dword [0xb8000], 0x2f592f53
	mov dword [0xb8004], 0x2f4f2f44
	mov word [0xb8008], 0x2f53

    ; Save Multiboot info for later use.
    mov [MULTIBOOT_MAGIC], eax
    mov [MULTIBOOT_INFO], ebx

	; Push Multiboot info structure and magic number.
	push ebx
	push eax

	; Call the early kernel main function.
	; This sets up early boot paging, etc.
	extern kernel_main_early
	call kernel_main_early

	; Pop Multiboot info off.
	pop eax
	pop ebx

	; Jump to higher half.
	lea edx, [_start_higherhalf]
	jmp edx

; Higher-half of our start function.
section .text
_start_higherhalf:
	; Point stack pointer to top.
	mov esp, stack_top
	mov ebp, stack_top
 
	; Ensure stack is 16-bit aligned.
    and esp, -16

	; Push Multiboot info structure and magic number.
	push ebx

	; Call the kernel main function!
	extern kernel_main
	call kernel_main
 
	; Disable interrupts and loop forever.
	cli
.hang:	hlt
	jmp .hang
