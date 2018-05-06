;
; File: lock.asm
; 
; Copyright (c) 2017-2018 Sydney Erickson, John Davis
; 
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;

[bits 32]
section .text

; https://wiki.osdev.org/Spinlock
; Spinlock lock function.

global spinlock_lock
spinlock_lock:
    ; Get EFLAGS register.
    pushfd
    pop edx
    push edx
    popfd

    ; Disable interrupts.
    cli

    ; Get lock object.
    mov eax, [esp+4]

.loop:
    ; Attempt to lock object.
    lock bts dword [eax], 0
    pause
	jc .loop

    ; Save state of interrupts.
    and edx, 0x200
    mov [eax+4], edx
	ret

global spinlock_release
spinlock_release:
    ; Get lock object.
    mov eax, [esp+4]



    ; Enable interrupts if they were enabled before.
    mov edx, [eax+4]

    ; Release lock.
    mov dword [eax], 0
    cmp edx, 0
    jz .spinlock_release_ret
    sti

.spinlock_release_ret:
    ret
