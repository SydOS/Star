[bits 64]
section .text

; https://wiki.osdev.org/Spinlock
; Spinlock lock function.

global spinlock_lock
spinlock_lock:
    ; Get lock object.
    mov rax, rdi

.loop:
    ; Attemp to lock object.
    lock bts qword [rax], 0
    pause
	jc .loop

    ; Get RFLAGS register.
    pushfq
    pop rdx
    push rdx
    popfq

    ; Save state of interrupts.
    and rdx, 0x200
    mov [rax+8], rdx

    ; Disable interrupts.
    cli
	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov rax, rdi

    ; Release lock.
    mov qword [rax], 0

    ; Enable interrupts if they were enabled before.
    mov rdx, [rax+8]
    cmp rdx, 0
    jz .spinlock_release_ret
    sti

.spinlock_release_ret:
    ret
