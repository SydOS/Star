; 32-bit code.
[bits 32]
section .text

; Empty IRQ handler for spurious IRQs.
global _irq_empty
_irq_empty:
    iretd

; IRQ common handler. This calls the handler defined in irqs.c.
extern irqs_handler
global _irq_common
_irq_common:
    ; The processor has already pushed SS, user ESP, EFLAGS, CS, and EIP to the stack.
    ; Push general registers (EAX, ECX, EDX, EBX, EBP, ESP, ESI, and EDI) to stack.
    pushad

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

    ; Push stack for use in C handler.
    mov eax, esp
    push eax

    ; Call IRQ C handler.
    call irqs_handler
    pop eax

global _irq_exit
_irq_exit:
; Restore segments.
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore general registers (EDI, ESI, ESP, EBP, EBX, EDX, ECX, and EAX).
    popad

    ; Continue execution.
    iretd
