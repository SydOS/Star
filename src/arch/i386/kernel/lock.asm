[bits 32]
section .text

; https://wiki.osdev.org/Spinlock
; Spinlock lock function.

global spinlock_lock
spinlock_lock:
    ; Get lock object.
    mov eax, [esp+4]

.loop:
    ; Attempt to lock object.
    lock bts dword [eax], 0
    pause
	jc .loop

    ; Get EFLAGS register.
    pushfd
    pop edx
    push edx
    popfd

    ; Disable interrupts if they are enabled.
    and edx, 0x200
    mov [eax+4], edx
    cmp edx, 0
    jz .spinlock_lock_ret
    cli

.spinlock_lock_ret:
	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov eax, [esp+4]

    ; Release lock.
    mov dword [eax], 0

    ; Enable interrupts if they were enabled before.
    mov edx, [eax+4]
    cmp edx, 0
    jz .spinlock_release_ret
    sti

.spinlock_release_ret:
    ret
