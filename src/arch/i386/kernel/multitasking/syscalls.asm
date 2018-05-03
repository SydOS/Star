; 32-bit code.
[bits 32]
section .text

extern syscalls_handler

global _syscalls_sysenter
_syscalls_sysenter:
    ; Save ECX, EDX.
    push ecx
    push edx

    ; Save segments.
    push ds
    push es
    push fs
    push gs
   
    ; Save ESP to ECX.
    mov ecx, esp

    ; Execute SYSENTER.
    sysenter

    ; We jump here when SYSENTER is done.
_syscalls_sysenter_done:
    ; Restore segments.
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore EDX and ECX.
    pop edx
    pop ecx
    ret

global _syscalls_sysenter_handler
_syscalls_sysenter_handler:
    ; At this point, the stack specified for SYSENTER has been changed to.
    ; Disable interrupts.
    cli

    ; Set up kernel segments.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Push caller's ESP (in ECX), EBP, EAX, EBX, ESI, and EDI to stack.
    push ecx
    push ebp
    push eax
    push ebx
    push esi
    push edi
    
    ; Get caller's stack.
    mov eax, ecx
    mov ecx, 7
    add eax, 24 ; Move past stuff pushed before the SYSENTER.
    add eax, (4*8) ; 7 arguments, each 4 bytes.

    ; Copy args to stack.
.loop_syse_args:
    sub eax, 4
    mov ebx, [eax]
    push ebx 
    dec ecx
    jnz .loop_syse_args

    ; Call C handler.
    call syscalls_handler

    ; Pop off args and discard.
    mov ecx, 7
.loop_syse_args_del:
    pop ebx
    dec ecx
    jnz .loop_syse_args_del

    ; Restore caller's EDI, ESI, EBX, EAX, and EBP.
    pop edi
    pop esi
    pop ebx
    pop eax
    pop ebp

    ; Restore caller's ESP into ECX and EIP into EDX for SYSEXIT to use.
    pop ecx
    mov edx, _syscalls_sysenter_done

    ; Re-enable interrupts and return to caller.
    sti
    sysexit

global _syscalls_interrupt
_syscalls_interrupt:
    ; Raise syscall interrupt. Passed args should be on stack already.
    ; Return value from handler will be in EAX.
    int 0x80
    ret

global _syscalls_interrupt_handler
_syscalls_interrupt_handler:
    ; The processor has already pushed SS, ESP, EFLAGS, CS, and EIP to the stack.
    ; Push general registers (EBX, ECX, EDX, EBP, ESI, and EDI) to stack.
    push ebx
    push ecx
    push edx
    push ebp
    push esi
    push edi

    ; Push segments to stack.
    push ds
    push es
    push fs
    push gs

    ; Set up kernel segments.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Get caller's ESP from our stack.
    ; 52 = number of bytes to go up stack until we get the ESP that was pushed prior.
    mov eax, [esp+52]
    mov ecx, 7      ; 7 arguments.
    add eax, (4*8)  ; 7 arguments, each 4 bytes.

    ; Copy args to stack.
.loop_int_args:
    sub eax, 4
    mov ebx, [eax]
    push ebx 
    dec ecx
    jnz .loop_int_args

    ; Call C handler.
    call syscalls_handler

    ; Pop off args and discard.
    mov ecx, 7
.loop_int_args_del:
    pop ebx
    dec ecx
    jnz .loop_int_args_del

    ; Restore segments.
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general registers (EDI, ESI, EBP, EDX, ECX, and EBX).
    pop edi
    pop esi
    pop ebp
    pop edx
    pop ecx
    pop ebx

    ; Continue execution. This restores EIP, CS, EFLAGS, ESP, and SS, and re-enables interrupts.
    ; Return value from handler is in EAX.
    iretd
