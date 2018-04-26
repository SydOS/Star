[bits 32]
section .text

; https://wiki.osdev.org/Spinlock
; Spinlock lock function.

global spinlock_lock
spinlock_lock:
    ; Get lock object.
    mov eax, [esp+4]

.loop:
    ; Attemp to lock object.
    lock bts dword [eax], 0
    pause
	jc .loop
	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov eax, [esp+4]

    ; Release lock.
    mov dword [eax], 0
    ret
