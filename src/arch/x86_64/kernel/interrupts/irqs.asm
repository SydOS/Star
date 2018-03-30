; 64-bit code.
[bits 64]
section .text

; Empty IRQ handler for spurious IRQs.
global _irq_empty
_irq_empty:
    iretq

; IRQ common handler. This calls the handler defined in irqs.c.
extern irqs_handler
global _irq_common
_irq_common:
    ; The processor has already pushed SS, RSP, RFLAGS, CS, and RIP to the stack.
    ; Push general registers (RAX, RCX, RDX, RBX, RBP, RSI, and RDI) to stack.
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi

    ; Push x64 registers (R15, R14, R13, R12, R11, R10, R9, and R8) to stack.
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    ; Push segments to stack.
    ; DS and ES cannot be directly pushed, so we must copy them to RAX first.
    mov rax, ds
    push rax
    mov rax, es
    push rax
    push fs
    push gs

    ; Set up kernel segments.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call IRQ C handler.
    mov rdi, rsp
    call irqs_handler

global _irq_exit
_irq_exit:
    ; Restore segments.
    ; DS and ES cannot be directly restored, so we must copy them to RAX first.
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore x64 registers (R8, R9, R10, R11, R12, R13, R14, R15) to stack.
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Restore general registers (RDI, RSI, RBP, RBX, RDX, RCX, and RAX).
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    ; Continue execution.
    iretq
