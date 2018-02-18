[bits 32]
[section .text]
 
global _enable_A20
_enable_A20:
        in al, 0x92
        or al, 2
        out 0x92, al

global _check
_check:
pushad
mov edi,0x112345  ;odd megabyte address.
mov esi,0x012345  ;even megabyte address.
mov [esi],esi     ;making sure that both addresses contain diffrent values.
mov [edi],edi     ;(if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
cmpsd             ;compare addresses to see if the're equivalent.
popad
jne A20_on        ;if not equivalent , A20 line is set.
mov eax, 0
ret               ;if equivalent , the A20 line is cleared.

A20_on:
mov eax, 1
ret