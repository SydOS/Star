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

global _detect_cpuid
_detect_cpuid:
	pushfd                               ;Save EFLAGS
    pushfd                               ;Store EFLAGS
    xor dword [esp],0x00200000           ;Invert the ID bit in stored EFLAGS
    popfd                                ;Load stored EFLAGS (with ID bit inverted)
    pushfd                               ;Store EFLAGS again (ID bit may or may not be inverted)
    pop eax                              ;eax = modified EFLAGS (ID bit may or may not be inverted)
    xor eax,[esp]                        ;eax = whichever bits were changed
    popfd                                ;Restore original EFLAGS
    and eax,0x00200000                   ;eax = zero if ID bit can't be changed, else non-zero
    ret

global _cpuid_string_supported
_cpuid_string_supported:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000004
	jl	_cpuid_string_supported_errorret
	xor eax, eax
	ret
_cpuid_string_supported_errorret:
	mov eax, 1
	ret

global _enable_protected_mode
_enable_protected_mode:
	mov eax, cr0 
	or al, 1       ; set PE (Protection Enable) bit in CR0 (Control Register 0)
	mov cr0, eax

	extern protected_mode_land
	call protected_mode_land

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