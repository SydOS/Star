; 64-bit code.
[bits 64]
section .text
extern tasking_kill_thread

; Thread execution shim.
global _tasking_thread_exec
_tasking_thread_exec:
    ; Get args in proper registers.
    ; RBX = arg1, C code expects RDI to be arg1.
    ; RCX = arg2, C code expects RSI to be arg2.
    ; RDX = arg3, C code expects RDX to be arg3.
    mov rdi, rbx
    mov rsi, rcx

    ; Clear now-unused registers.
    xor rbx, rbx
    xor rcx, rcx

    ; Execute the thread's function.
    ; The function address was put into RAX when the thread was created.
    call rax

    ; Kill thread and wait to die.
    call tasking_kill_thread
.loop:
    jmp .loop
