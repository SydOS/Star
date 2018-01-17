MBALIGN  equ  1<<0
MEMINFO  equ  1<<1
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM
 
; Allocate stack
section .bss
align 16
stack_bottom:
resb 16384
stack_top:
 

section .text
global _start:function (_start.end - _start)
_start:
	; point stack pointer to top
	mov esp, stack_top
 
	extern kernel_main
	call kernel_main
 
	cli
.hang:	hlt
	jmp .hang
.end: