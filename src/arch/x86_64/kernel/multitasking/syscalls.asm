; 64-bit code.
[bits 64]
section .text
extern SyscallStack
extern SyscallTemp

global _syscalls_syscall
_syscalls_syscall:
    syscall
    ret

global _syscalls_handler
_syscalls_handler:
    ; Disable interrupts.
    cli

    ; Save RAX to temp location so we can store RSP there.
    mov [qword SyscallTemp], rax
    mov rax, rsp

    ; Load up SYSCALL handler stack.
    mov rsp, SyscallStack
    add rsp, 4096

    ; Push caller's RSP, RBP, RIP, and RFLAGS in R11 to stack.
    push rax
    push rbp
    push rcx
    push r11
    xor rbp, rbp

    ; Save other registers.
    push r12
    push r13
    push r14
    push r15
    push rbx

    ; Restore RAX.
    mov rax, [qword SyscallTemp]
    mov rcx, r10

    ; Call C handler.
    extern syscalls_handler
    call syscalls_handler

    ; Restore other registers.
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

