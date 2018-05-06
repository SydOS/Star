;
; File: exceptions.asm
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

;
; Exceptions.
;
; 0: Divide By Zero Exception.
global _exception0
_exception0:
    cli
    push byte 0 ; No error code.
    push byte 0 ; Exception number.
    jmp exception_common_stub

; 1: Debug Exception.
global _exception1
_exception1:
    cli
    push byte 0 ; No error code.
    push byte 1 ; Exception number.
    jmp exception_common_stub

; 2: Non Maskable Interrupt Exception.
global _exception2
_exception2:
    cli
    push byte 0 ; No error code.
    push byte 2 ; Exception number.
    jmp exception_common_stub

; 3: Breakpoint Exception.
global _exception3
_exception3:
    cli
    push byte 0 ; No error code.
    push byte 3 ; Exception number.
    jmp exception_common_stub

; 4: Overflow Exception.
global _exception4
_exception4:
    cli
    push byte 0 ; No error code.
    push byte 4 ; Exception number.
    jmp exception_common_stub

; 5: Bound Range Exceeded Exception.
global _exception5
_exception5:
    cli
    push byte 0 ; No error code.
    push byte 5 ; Exception number.
    jmp exception_common_stub

; 6: Invalid Opcode Exception.
global _exception6
_exception6:
    cli
    push byte 0 ; No error code.
    push byte 6 ; Exception number.
    jmp exception_common_stub

; 7: Device Not Available Exception.
global _exception7
_exception7:
    cli
    push byte 0 ; No error code.
    push byte 7 ; Exception number.
    jmp exception_common_stub

; 8: Double Fault Exception.
global _exception8
_exception8:
    cli
    push byte 8 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

;  9: Coprocessor Segment Overrun (unused in modern processors).
global _exception9
_exception9:
    cli
    push byte 0 ; No error code.
    push byte 9 ; Exception number.
    jmp exception_common_stub

; 10: Invalid TSS Exception.
global _exception10
_exception10:
    cli
    push byte 10 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

; 11: Segment Not Present.
global _exception11
_exception11:
    cli
    push byte 11 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

; 12: Stack Fault Exception.
global _exception12
_exception12:
    cli
    push byte 12 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

; 13: General Protection Exception.
global _exception13
_exception13:
    cli
    push byte 13 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

; 14: Page-Fault Exception.
global _exception14
_exception14:
    cli
    push byte 14 ; Exception number. Error code is pushed by system.
    jmp exception_common_stub

; 16: x87 FPU Floating-Point Error.
global _exception16
_exception16:
    cli
    push byte 0 ; No error code.
    push byte 16 ; Exception number.
    jmp exception_common_stub

; 17: Alignment Check Exception.
global _exception17
_exception17:
    cli
    push byte 0 ; No error code.
    push byte 17 ; Exception number.
    jmp exception_common_stub

; 18: Machine-Check Exception.
global _exception18
_exception18:
    cli
    push byte 0 ; No error code.
    push byte 18 ; Exception number.
    jmp exception_common_stub

; 19: SIMD Floating-Point Exception.
global _exception19
_exception19:
    cli
    push byte 0 ; No error code.
    push byte 19 ; Exception number.
    jmp exception_common_stub

; 20: Virtualization Exception.
global _exception20
_exception20:
    cli
    push byte 0 ; No error code.
    push byte 20 ; Exception number.
    jmp exception_common_stub

;
; Exceptions common stub. This calls the handler defined in exceptions.c.
;
extern exceptions_handler
exception_common_stub:
    ; The processor has already pushed SS, RSP, RFLAGS, CS, and RIP to the stack.
    ; The interrupt-specific handler also pushed the interrupt number, and an empty error code if needed.

    ; Push general registers (RAX, RCX, RDX, RBX, RBP, RSI, and RDI) to stack.
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi

    ; Push x64 registers (R15, R14, R13, R12, R11, R10, R9, and R8) to stack.
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    ; Push segments to stack.
    ; DS and ES cannot be directly pushed, so we must copy them to RAX first.
    mov rax, ds
    push rax
    mov rax, es
    push rax
    push fs
    push gs

    ; Set up kernel segments.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C exceptions handler.
    mov rdi, rsp
    call exceptions_handler

    ; Restore segments.
    ; DS and ES cannot be directly restored, so we must copy them to RAX first.
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    ; Restore x64 registers (R8, R9, R10, R11, R12, R13, R14, R15) to stack.
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; Restore general registers (RDI, RSI, RBP, RBX, RDX, RCX, and RAX).
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    ; Move past the error code and exception number, and continue execution.
    add rsp, 16
    iretq
