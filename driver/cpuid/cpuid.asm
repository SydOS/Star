section .text

global _cpuid_detect
_cpuid_detect:
	pushfd                               ;Save EFLAGS
    pushfd                               ;Store EFLAGS
    xor dword [esp],0x00200000           ;Invert the ID bit in stored EFLAGS
    popfd                                ;Load stored EFLAGS (with ID bit inverted)
    pushfd                               ;Store EFLAGS again (ID bit may or may not be inverted)
    pop eax                              ;eax = modified EFLAGS (ID bit may or may not be inverted)
    xor eax,[esp]                        ;eax = whichever bits were changed
    popfd                                ;Restore original EFLAGS
    and eax,0x00200000                   ;eax = zero if ID bit can't be changed, else non-zero
    ret

;
; EAX=0x0
; https://en.wikipedia.org/wiki/CPUID#EAX=0:_Highest_Function_Parameter
;
; Gets the highest CPUID function supported by the processor.
global _cpuid_gethighestfunction
_cpuid_gethighestfunction:
    mov eax, 0x0
    cpuid
    ret

;
; EAX=0x1
; https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
;
; Gets the processor signature.
global _cpuid_getsignature
_cpuid_getsignature:
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov eax, 0x1
    cpuid
    ret

; Gets misc. info about the processor.
global _cpuid_getmiscinfo
_cpuid_getmiscinfo:
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov eax, 0x1
    cpuid
    mov eax, ebx
    ret

; Gets the original set of processor features.
global _cpuid_getfeatures
_cpuid_getfeatures:
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov eax, 0x1
    cpuid
    mov eax, edx
    ret

; Gets the second set of processor features.
global _cpuid_getfeatures2
_cpuid_getfeatures2:
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov eax, 0x1
    cpuid
    mov eax, ecx
    ret

;
; EAX=0x80000000
; https://en.wikipedia.org/wiki/CPUID#EAX=80000000h:_Get_Highest_Extended_Function_Supported
;
; Gets the highest extended CPUID function supported by the processor.
global _cpuid_gethighestextendedfunction
_cpuid_gethighestextendedfunction:
    mov eax, 0x80000000
    cpuid
    ret

;
; EAX=0x80000001
; https://en.wikipedia.org/wiki/CPUID#EAX=80000001h:_Extended_Processor_Info_and_Feature_Bits
;
; Gets extended feature flags.
global _cpuid_getextendedprocessorfeatures
_cpuid_getextendedprocessorfeatures:
    mov eax, 80000001
    cpuid
    mov eax, edx
    ret

global _cpuid_getextendedprocessorfeatures2
_cpuid_getextendedprocessorfeatures2:
    mov eax, 80000001
    cpuid
    mov eax, ecx
    ret
