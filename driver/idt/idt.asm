.global __idt_default_handler
.type __idt_default_handler, @function
__idt_default_handler:
	pushal
	mov $0x20, %al
	mov $0x20, %dx
	out %al, %dx
	#call _test
	popal
	iretl

.global _set_idtr
.type _set_idtr, @function
_set_idtr:
	push %ebp
	movl %esp, %ebp

	lidt 0x10F0

	movl %ebp, %esp
	pop %ebp
	ret