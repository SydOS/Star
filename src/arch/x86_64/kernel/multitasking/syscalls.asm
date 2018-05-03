; 64-bit code.
[bits 64]
section .text
extern SyscallStack

extern syscalls_handler

global _syscalls_syscall
_syscalls_syscall:
    ; Execute SYSCALL. Passed args should be in proper registers already.
    ; Return value from handler will be in RAX.
    syscall
    ret

global _syscalls_syscall_handler
_syscalls_syscall_handler:
    ; Disable interrupts.
    cli

    ; Save caller's RSP to RAX.
    mov rax, rsp

    ; Load up SYSCALL handler stack.
    mov rsp, SyscallStack
    add rsp, 4096

    ; Push caller's RSP (in RAX), RBP, RIP (in RCX), and RFLAGS (in R11) to stack.
    push rax
    push rbp
    push rcx
    push r11

    ; Save unused registers.
    push r12
    push r13
    push r14
    push r15
    push rbx

    ; Get index argument and push to our stack.
    mov rbx, [rax+8]
    push rbx

    ; Call C handler. Afterwards, discard the index argument from stack.
    call syscalls_handler
    pop rbx

    ; Restore unused registers.
    pop rbx
    pop r15
    pop r14
    pop r13
    pop r12

    ; Restore caller's RFLAGS, RIP, RBP, and RSP.
    pop r11
    pop rcx
    pop rbp
    pop rsp

    ; Restore control back to the calling code.
    o64 sysret

global _syscalls_interrupt
_syscalls_interrupt:
    ; Raise syscall interrupt. Passed args should be in proper registers already.
    ; Return value from handler will be in RAX.
    int 0x80
    ret

global _syscalls_interrupt_handler
_syscalls_interrupt_handler:
    ; The processor has already pushed SS, RSP, RFLAGS, CS, and RIP to the stack.
    ; Push unused general registers (RBX and RBP) to stack.
    push rbx
    push rbp

    ; Push unused x64 registers (R15, R14, R13, R12, R11, and R10) to stack.
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10

    ; Push segments to stack.
    ; DS and ES cannot be directly pushed, so we must copy them to RAX first.
    mov rbx, ds
    push rbx
    mov rbx, es
    push rbx
    push fs
    push gs

    ; Set up kernel segments.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Get caller's RSP from our stack.
    ; 120 = number of bytes to go up stack until we get the RSP that was pushed prior.
    mov rax, [rsp+120]

    ; Get index argument and push to our stack.
    mov rbx, [rax+8]
    push rbx

    ; Call C handler. Afterwards, discard the index argument from stack.
    call syscalls_handler
    pop rbx

    ; Restore segments.
    ; DS and ES cannot be directly restored, so we must copy them to RAX first.
    pop gs
    pop fs
    pop rbx
    mov es, bx
    pop rbx
    mov ds, bx

    ; Restore unused x64 registers (R10, R11, R12, R13, R14, R15) to stack.
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Restore unused general registers (RBP and RBX).
    pop rbp
    pop rbx

    ; Continue execution. This restores RIP, CS, RFLAGS, RSP, and SS, and re-enables interrupts.
    ; Return value from handler is in RAX.
    iretq
