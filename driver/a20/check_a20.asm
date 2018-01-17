[bits 16]
 
; Function: check_a20
;
; Purpose: to check the status of the a20 line in a completely self-contained state-preserving way.
;          The function can be modified as necessary by removing push's at the beginning and their
;          respective pop's at the end if complete self-containment is not required.
;
; Returns: 0 in ax if the a20 line is disabled (memory wraps around)
;          1 in ax if the a20 line is enabled (memory does not wrap around)
 
global check_a20
check_a20:
    pushf
    push ds
    push es
    push di
    push si
 
    cli
 
    xor ax, ax ; ax = 0
    mov es, ax
 
    not ax ; ax = 0xFFFF
    mov ds, ax
 
    mov di, 0x0500
    mov si, 0x0510
 
    mov al, byte [es:di]
    push ax
 
    mov al, byte [ds:si]
    push ax
 
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
 
    cmp byte [es:di], 0xFF
 
    pop ax
    mov byte [ds:si], al
 
    pop ax
    mov byte [es:di], al
 
    mov ax, 0
    je check_a20__exit
 
    mov ax, 1
 
check_a20__exit:
    pop si
    pop di
    pop es
    pop ds
    popf
 
    ret