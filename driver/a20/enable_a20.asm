[bits 32]
[section .text]
 
global _enable_A20
_enable_A20:
        in al, 0x92
        or al, 2
        out 0x92, al