; 64-bit code.
[bits 64]
section .text
extern SyscallStack

extern syscalls_handler
extern gdt_tss_get
extern gdt_tss_get_kernel_stack

global _syscalls_syscall
_syscalls_syscall:
    ; Push non-arg registers to stack.
    push rbx
    push rbp
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Push args to stack.
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9

    ; Execute SYSCALL. Passed args should be in proper registers already.
    ; Return value from handler will be in RAX.
    syscall

    ; Pop args from stack.
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi

    ; Restore non-arg registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop rbp
    pop rbx
    ret

global _syscalls_syscall_handler
_syscalls_syscall_handler:
    ; Disable interrupts.
    cli

    ; Push caller's RIP (in RCX) and RFLAGS (in R11) to caller's stack.
    push rcx
    push r11

    ; Get current TSS and ring 0 stack.
    call gdt_tss_get
    mov rdi, rax
    call gdt_tss_get_kernel_stack

    ; Restore caller's RFLAGS into R11 and RIP into R10.
    pop r11
    pop r10
    
    ; Save caller's RSP to RBX.
    mov rbx, rsp

    ; Restore args into proper registers.
    mov r9, [rsp]
    mov r8, [rsp+8]
    mov rcx, [rsp+16]
    mov rdx, [rsp+24]
    mov rsi, [rsp+32]
    mov rdi, [rsp+40]
    
    ; Load up SYSCALL handler stack and push caller's RSP (in RBX) to our stack.
    mov rsp, [rax]
    push rbx

    ; Push caller's RIP (in R10) and RFLAGS (in R11) to our stack.
    push r10
    push r11

    ; Get index argument and push to our stack.
    ; 72 = 14 registers plus 8 bytes, location of index.
    mov rbx, [rbx+120]
    push rbx

    ; Call C handler. Afterwards, discard the index argument from stack.
    call syscalls_handler
    pop rbx

    ; Restore caller's RFLAGS to R11 and RIP to RCX.
    pop r11
    pop rcx

    ; Restore caller's RSP.
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
