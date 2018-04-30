; 32-bit code.
[bits 32]
section .text
extern tasking_kill_thread

; Thread execution shim.
global _tasking_thread_exec
_tasking_thread_exec:
    ; Reset ESP. EBP contains the top of the stack.
    mov esp, ebp

    ; Push args (EBX = arg1, ECX = arg2, EDX = arg3) to stack.
    push edx
    push ecx
    push ebx

    ; Clear now-unused registers.
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    ; Execute the thread's function.
    ; The function address was put into EAX when the thread was created.
    call eax

    ; Kill thread and wait to die.
    call tasking_kill_thread
.loop:
    jmp .loop
