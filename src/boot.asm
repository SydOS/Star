[bits 32]

MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .multiboot
align 4
; Multiboot header signature.
dd MULTIBOOT_HEADER_MAGIC
dd MULTIBOOT_HEADER_FLAGS
dd MULTIBOOT_CHECKSUM
 
; Allocate stack
section .bss
align 16
stack_bottom:
	resb 16384
stack_top:
 

section .text

;global _enable_protected_mode
;_enable_protected_mode:
;	mov eax, cr0 
;	or al, 1       ; set PE (Protection Enable) bit in CR0 (Control Register 0)
;	mov cr0, eax

;	extern protected_mode_land
;	call protected_mode_land

global _start:function (_start.end - _start)
_start:
	; Point stack pointer to top.
	mov esp, stack_top
 
	; Ensure stack is 16-bit aligned.
    and esp, -16

	; Push Multiboot info structure and magic number.
	push ebx
	push eax

	; Call the kernel main function!
	extern kernel_main
	call kernel_main
 
	cli
.hang:	hlt
	jmp .hang
.end:
