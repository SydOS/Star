;
; File: tasking.asm
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

; 64-bit code.
[bits 64]
section .text
extern tasking_cleanup

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

    ; Cleanup thread and wait to die.
    call tasking_cleanup
.loop:
    jmp .loop
