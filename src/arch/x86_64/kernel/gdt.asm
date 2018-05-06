[bits 64]
section .text

; Sets up the new GDT into the processor while flushing out the old one.
global _gdt_load
_gdt_load:
	; Load the GDT.
	lgdt [rdi]

	; Load segments.
	mov rax, rsi
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret

; Loads the TSS into the processor.
global _gdt_tss_load
_gdt_tss_load:
	; Load specified TSS segment into the processor.
	mov rax, rdi
	ltr ax
	ret
