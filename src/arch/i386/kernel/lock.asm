[bits 32]
section .text

; https://wiki.osdev.org/Spinlock
; Spinlock lock function.

global spinlock_lock
spinlock_lock:
    ; Get lock object.
    mov eax, [esp+4]

.loop:

	bt dword[eax], 0

	jc .loop



	lock bts dword[eax], 0

	jc .loop



	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov eax, [esp+4]

    ; Release lock.
    btr dword [eax], 0
    ret
