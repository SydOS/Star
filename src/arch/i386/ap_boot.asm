extern gdtPtr

KERNEL_VIRTUAL_OFFSET equ 0xC0000000
gdtPhys equ gdtPtr - KERNEL_VIRTUAL_OFFSET
;_ap_bootstrap_protected_real equ _ap_bootstrap_protected - 0xC0000000

; Allocate stack
section .bss
	align 16
stack_bottom:
	resb 4096
stack_top:

section .text
[bits 16]

global _ap_bootstrap_init
global _ap_bootstrap_end
_ap_bootstrap_init:
    ; Ensure interrupts are disabled.
    cli

    ; Enable A20 gate.
    in al, 0x92
    or al, 2
    out 0x92, al
    xchg bx, bx
    ; Load GDT that was setup earlier in boot.
    mov eax, gdtPhys
    lgdt [eax]
    hlt
    ; Enable protected mode.
    mov eax, cr0
    or al, 1
    mov cr0, eax
    hlt
    ; Far jump into protected mode.
   ; jmp 0x08:dword _ap_bootstrap_protected_real

;[bits 32]
;_ap_bootstrap_protected:
;    hlt

_ap_bootstrap_end:
_ap_bootstrap_size equ $ - _ap_bootstrap_init - 1
