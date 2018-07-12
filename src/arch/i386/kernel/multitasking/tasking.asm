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

; 32-bit code.
[bits 32]
section .text
extern tasking_cleanup

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
    call tasking_cleanup
.loop:
    jmp .loop
