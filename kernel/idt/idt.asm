; Loads the IDT into the processor.
global _idt_load
_idt_load:
    mov eax, [esp+4] ; Get the pointer to the IDT, passed as a parameter. 
    lidt [eax]       ; Load the IDT pointer.
    ret
