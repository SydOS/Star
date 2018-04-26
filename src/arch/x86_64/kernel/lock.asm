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
	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov rax, rdi

    ; Release lock.
    mov qword [rax], 0
    ret
