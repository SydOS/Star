[bits 64]
section .text

;
; Exception ISRs.
;
; 0: Divide By Zero Exception.
global _isr0
_isr0:
    cli
    push byte 0 ; No error code.
    push byte 0 ; Exception number.
    jmp isr_common_stub

; 1: Debug Exception.
global _isr1
_isr1:
    cli
    push byte 0 ; No error code.
    push byte 1 ; Exception number.
    jmp isr_common_stub

; 2: Non Maskable Interrupt Exception.
global _isr2
_isr2:
    cli
    push byte 0 ; No error code.
    push byte 2 ; Exception number.
    jmp isr_common_stub

; 3: Breakpoint Exception.
global _isr3
_isr3:
    cli
    push byte 0 ; No error code.
    push byte 3 ; Exception number.
    jmp isr_common_stub

; 4: Overflow Exception.
global _isr4
_isr4:
    cli
    push byte 0 ; No error code.
    push byte 4 ; Exception number.
    jmp isr_common_stub

; 5: Bound Range Exceeded Exception.
global _isr5
_isr5:
    cli
    push byte 0 ; No error code.
    push byte 5 ; Exception number.
    jmp isr_common_stub

; 6: Invalid Opcode Exception.
global _isr6
_isr6:
    cli
    push byte 0 ; No error code.
    push byte 6 ; Exception number.
    jmp isr_common_stub

; 7: Device Not Available Exception.
global _isr7
_isr7:
    cli
    push byte 0 ; No error code.
    push byte 7 ; Exception number.
    jmp isr_common_stub

; 8: Double Fault Exception.
global _isr8
_isr8:
    cli
    push byte 8 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

;  9: Coprocessor Segment Overrun (unused in modern processors).
global _isr9
_isr9:
    cli
    push byte 0 ; No error code.
    push byte 9 ; Exception number.
    jmp isr_common_stub

; 10: Invalid TSS Exception.
global _isr10
_isr10:
    cli
    push byte 10 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

; 11: Segment Not Present.
global _isr11
_isr11:
    cli
    push byte 11 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

; 12: Stack Fault Exception.
global _isr12
_isr12:
    cli
    push byte 12 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

; 13: General Protection Exception.
global _isr13
_isr13:
    cli
    push byte 13 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

; 14: Page-Fault Exception.
global _isr14
_isr14:
    cli
    push byte 14 ; Exception number. Error code is pushed by system.
    jmp isr_common_stub

; 16: x87 FPU Floating-Point Error.
global _isr16
_isr16:
    cli
    push byte 0 ; No error code.
    push byte 16 ; Exception number.
    jmp isr_common_stub

; 17: Alignment Check Exception.
global _isr17
_isr17:
    cli
    push byte 0 ; No error code.
    push byte 17 ; Exception number.
    jmp isr_common_stub

; 18: Machine-Check Exception.
global _isr18
_isr18:
    cli
    push byte 0 ; No error code.
    push byte 18 ; Exception number.
    jmp isr_common_stub

; 19: SIMD Floating-Point Exception.
global _isr19
_isr19:
    cli
    push byte 0 ; No error code.
    push byte 19 ; Exception number.
    jmp isr_common_stub

; 20: Virtualization Exception.
global _isr20
_isr20:
    cli
    push byte 0 ; No error code.
    push byte 20 ; Exception number.
    jmp isr_common_stub

;
; IRQ ISRs.
;
; 32: IRQ0 (Timer).
global _irq0
_irq0:
    cli
    push byte 0 ; No error code.
    push byte 32 ; Interrupt number.
    jmp isr_common_stub

; 33: IRQ1 (PS/2 keyboard).
global _irq1
_irq1:
    cli
    push byte 0 ; No error code.
    push byte 33 ; Interrupt number.
    jmp isr_common_stub

; 34: IRQ2 (slave PIC).
global _irq2
_irq2:
    cli
    push byte 0 ; No error code.
    push byte 34 ; Interrupt number.
    jmp isr_common_stub

; 35: IRQ3 (serial port COM2).
global _irq3
_irq3:
    cli
    push byte 0 ; No error code.
    push byte 35 ; Interrupt number.
    jmp isr_common_stub

; 36: IRQ4 (serial port COM1).
global _irq4
_irq4:
    cli
    push byte 0 ; No error code.
    push byte 36 ; Interrupt number.
    jmp isr_common_stub

; 37: IRQ5 (parallel port LPT2).
global _irq5
_irq5:
    cli
    push byte 0 ; No error code.
    push byte 37 ; Interrupt number.
    jmp isr_common_stub

; 38: IRQ6 (floppy drive).
global _irq6
_irq6:
    cli
    push byte 0 ; No error code.
    push byte 38 ; Interrupt number.
    jmp isr_common_stub

; 39: IRQ7 (parallel port LPT1).
global _irq7
_irq7:
    cli
    push byte 0 ; No error code.
    push byte 39 ; Interrupt number.
    jmp isr_common_stub

; 40: IRQ8 (CMOS/real-time clock).
global _irq8
_irq8:
    cli
    push byte 0 ; No error code.
    push byte 40 ; Interrupt number.
    jmp isr_common_stub

; 41: IRQ9 (ACPI).
global _irq9
_irq9:
    cli
    push byte 0 ; No error code.
    push byte 41 ; Interrupt number.
    jmp isr_common_stub

; 42: IRQ10.
global _irq10
_irq10:
    cli
    push byte 0 ; No error code.
    push byte 42 ; Interrupt number.
    jmp isr_common_stub

; 43: IRQ11.
global _irq11
_irq11:
    cli
    push byte 0 ; No error code.
    push byte 43 ; Interrupt number.
    jmp isr_common_stub

; 44: IRQ12 (PS/2 mouse).
global _irq12
_irq12:
    cli
    push byte 0 ; No error code.
    push byte 44 ; Interrupt number.
    jmp isr_common_stub

; 45: IRQ13 (FPU).
global _irq13
_irq13:
    cli
    push byte 0 ; No error code.
    push byte 45 ; Interrupt number.
    jmp isr_common_stub

; 46: IRQ14 (primary ATA controller).
global _irq14
_irq14:
    cli
    push byte 0 ; No error code.
    push byte 46 ; Interrupt number.
    jmp isr_common_stub

; 47: IRQ15 (secondary ATA controller).
global _irq15
_irq15:
    cli
    push byte 0 ; No error code.
    push byte 47 ; Interrupt number.
    jmp isr_common_stub

;
; PCI APIC interrupt handlers.
;
; PCI interrupt PIN A (used with APIC).
global _irq_pcia
_irq_pcia:
    cli
    push byte 255 ; PCI interrupt.
    push byte 1 ; PCI interrupt pin.
    jmp isr_common_stub

; PCI interrupt PIN B (used with APIC).
global _irq_pcib
_irq_pcib:
    cli
    push byte 255 ; PCI interrupt.
    push byte 2 ; PCI interrupt pin.
    jmp isr_common_stub

; PCI interrupt PIN C (used with APIC).
global _irq_pcic
_irq_pcic:
    cli
    push byte 255 ; PCI interrupt.
    push byte 3 ; PCI interrupt pin.
    jmp isr_common_stub

; PCI interrupt PIN D (used with APIC).
global _irq_pcid
_irq_pcid:
    cli
    push byte 255 ; PCI interrupt.
    push byte 4 ; PCI interrupt pin.
    jmp isr_common_stub

;
; ISR common stub. This calls the handler defined in interrupts.c.
;
extern interrupts_isr_handler
isr_common_stub:
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

    ; Call C handler.
    mov rdi, rsp
    call interrupts_isr_handler

global _isr_exit
_isr_exit:
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

    ; Move past the error code and interrupt number and continue execution.
    add rsp, 16
    iretq

; Empty ISR handler for spurious interrupts.
global _isr_empty
_isr_empty:
    iretq
