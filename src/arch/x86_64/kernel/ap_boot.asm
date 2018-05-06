; Constants. These should match the ones in smp.h.
SMP_GDT32_ADDRESS equ 0x5A0
SMP_GDT64_ADDRESS equ 0x600
SMP_PAGING_PML4   equ 0x7000

; Offset the two jump functions to get the physical address.
_ap_bootstrap_protected_real equ _ap_bootstrap_protected - 0xFFFF800000000000
_ap_bootstrap_long_real equ _ap_bootstrap_long - 0xFFFF800000000000

; Allocate stack
section .bss
	align 32
ap_bootstrap_stack:
	resb 4096
ap_bootstrap_stack_end:

section .text

; 16-bit bootstrap code
[bits 16]

global _ap_bootstrap_init
global _ap_bootstrap_end
_ap_bootstrap_init:
    ; Ensure interrupts are disabled.
    cli

    ; Check if A20 line is enabled.
    call _check_a20

    ; Enable A20 gate if needed.
    cmp eax, 1
    je _ap_bootstrap_a20_enabled

    ; If we get here, A20 line needs enabling.
    call _enable_A20

_ap_bootstrap_a20_enabled:
    ; Load 32-bit GDT that was setup earlier in boot.
    mov eax, SMP_GDT32_ADDRESS
    lgdt [eax]

    ; Enable protected mode.
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump into protected mode.
    jmp 0x08:dword _ap_bootstrap_protected_real

; Checks A20 line status.
; 0 = disabled; 1 = enabled.
_check_a20:
    pushf
    push ds
    push es
    push di
    push si
 
    xor ax, ax ; ax = 0
    mov es, ax
 
    not ax ; ax = 0xFFFF
    mov ds, ax
 
    mov di, 0x0500
    mov si, 0x0510
 
    mov al, byte [es:di]
    push ax
 
    mov al, byte [ds:si]
    push ax
 
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
 
    cmp byte [es:di], 0xFF
 
    pop ax
    mov byte [ds:si], al
 
    pop ax
    mov byte [es:di], al
 
    mov ax, 0
    je _check_a20_exit
    mov ax, 1
 
_check_a20_exit:
    pop si
    pop di
    pop es
    pop ds
    popf
 
    ret

_enable_A20:
    call _a20wait
    mov al,0xAD
    out 0x64,al

    call _a20wait
    mov al,0xD0
    out 0x64, al

    call _a20wait2
    in al, 0x60
    push eax

    call _a20wait
    mov al, 0xD1
    out 0x64, al

    call _a20wait
    pop eax
    or al, 2
    out 0x60, al

    call _a20wait
    mov al, 0xAE
    out 0x64, al

    call _a20wait
    ret
 
_a20wait:
    in al, 0x64
    test al, 2
    jnz _a20wait
    ret
 
_a20wait2:
    in al, 0x64
    test al, 1
    jz _a20wait2
    ret


; 32-bit bootstrap code.
[bits 32]
_ap_bootstrap_protected:
    ; Set up segments.
    mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

    ; Set up paging using the root paging structure
    ; thats already set up.
    mov eax, SMP_PAGING_PML4
    mov cr3, eax

    ; Enable PAE.
    mov eax, cr4
    or eax, 0x00000020
    mov cr4, eax

    ; Enable IA-32e mode.
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    ; Enable paging.
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Load 64-bit GDT that was setup earlier in boot.
    mov eax, SMP_GDT64_ADDRESS
    lgdt [eax]

    ; Jump to 64-bit.
    jmp 0b1000:_ap_bootstrap_long_real

_ap_bootstrap_end:
_ap_bootstrap_size equ $ - _ap_bootstrap_init - 1

; 64-bit bootstrap code.
[bits 64]
_ap_bootstrap_long:
    ; load 0x10 into all data segment registers
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Jump to higher half.
    mov rax, _ap_bootstrap_higherhalf
    jmp rax

_ap_bootstrap_higherhalf:
    ; Set up temporary stack.
    mov rsp, ap_bootstrap_stack_end
    mov rbp, ap_bootstrap_stack_end

    ; Get the ID for this processor's LAPIC. ID is placed in RAX.
    extern lapic_id
    call lapic_id

    ; Get stack address. Address is placed in RAX.
    mov rdi, rax
    extern smp_ap_get_stack
    call smp_ap_get_stack

    ; Load up stack for this processor.
    mov rsp, rax
    add rsp, 0x4000
    mov rbp, rsp

    ; Pop into C code.
    extern smp_ap_main
    call smp_ap_main

    ; Never should get here, but if we do halt the processor.
    jmp $
    hlt
