[bits 32]
extern gdt32Ptr
section .text

; Sets up the new GDT into the processor while flushing out the old one.
global _gdt_load
_gdt_load:
	; Load the GDT.
	mov eax, gdt32Ptr
	lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	
	jmp 0x08:.flush2
.flush2:
	ret
